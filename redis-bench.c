#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <hiredis/hiredis.h>

typedef struct redis_key_range {
	size_t start_key;
	size_t end_key;
	size_t current;
} redisKeyRange;

void redisInitRange(redisKeyRange *range, size_t start, size_t length)
{
	range->start_key = start;
	range->current = start;
	range->end_key = start + length;
}

void redisResetRange(redisKeyRange *range)
{
	range->current = range->start_key;
}

int redisNextKey(redisKeyRange *range, size_t *key)
{
	if (range->current < range->end_key) {
		*key = range->current++;
		return 1;
	}
	return 0;
}

typedef struct redis_thread_work {
	pthread_mutex_t lock;
	char *host;
	char *blob;
	size_t gibs;
	size_t total_bytes;
	size_t last_key;
} redisWork;

#define RANGE_LENGTH 256
int redisNextKeyRange(redisWork *w, redisKeyRange *r)
{
	size_t start_key;

	if (w->total_bytes >= w->gibs)
		return 0;

	pthread_mutex_lock(&w->lock);
	start_key = w->last_key;
	w->last_key += RANGE_LENGTH;
	pthread_mutex_unlock(&w->lock);

	redisInitRange(r, start_key, RANGE_LENGTH);
	return 1;
}

void redisUpdateBytes(redisWork *w, size_t bytes)
{
	pthread_mutex_lock(&w->lock);
	w->total_bytes += bytes;
	pthread_mutex_unlock(&w->lock);
}

void parse_args(int argc, char **argv, char **host, int *flushall,
		size_t *gibs, int *nr_threads)
{
	int opt;

	while ((opt = getopt(argc, argv, "h:fs:t:")) != -1) {
		switch (opt) {
                case 'h':
			*host = optarg;
                        break;
                case 's':
                        *gibs = strtoull(optarg, 0, 10);
                        break;
                case 't':
                        *nr_threads = atoi(optarg);
                        break;
		case 'f':
			*flushall = 1;
			break;
                default:
                        fprintf(stderr, "Option not recognized!\n");
                        exit(EXIT_FAILURE);
                }
        }
}

void *redisThread(void *arg)
{
	redisContext *redisCtx;
	redisReply *redisReply;
	redisKeyRange range;
	redisWork *w = arg;
	size_t total_bytes;
	size_t key;

	redisCtx = redisConnect(w->host, 6379);
	if (redisCtx == NULL || redisCtx->err) {
		if (redisCtx)
			fprintf(stderr, "Error: %s\n", redisCtx->errstr);
		return 0;
	}

	while (redisNextKeyRange(w, &range)) {
		total_bytes = 0;
		while (redisNextKey(&range, &key)) {
			size_t bytes = rand() % 4096;
			redisAppendCommand(redisCtx, "SET %lu %b",
					   key, w->blob, bytes);
			total_bytes += bytes;
		}

		redisUpdateBytes(w, total_bytes);

		redisResetRange(&range);
		while (redisNextKey(&range, &key)) {
			if (redisGetReply(redisCtx,(void *)&redisReply) == REDIS_ERR) {
				fprintf(stderr, "Error: key=%lu: %s\n",
					key, redisCtx->errstr);
				return 0;
			}
			freeReplyObject(redisReply);
		}
	}
	redisFree(redisCtx);
	return 0;
}

int main(int argc, char **argv)
{
	char redisBlob[4096];
	char *host = "192.168.100.5";
	size_t gibs = 10;
	int nr_threads = 1;
	int flushall = 0;
	redisContext *redisCtx;
	redisWork work;
	pthread_t *threads;
	time_t stime, elapsed;
	int ret, i;

	srand(time(NULL));
	parse_args(argc, argv, &host, &flushall, &gibs, &nr_threads);

	pthread_mutex_init(&work.lock, NULL);
	work.gibs = gibs << 30; /* convert to bytes */
	work.total_bytes = 0;
	work.last_key = 0;
	work.host = host;
	work.blob = redisBlob;

	redisCtx = redisConnect(host, 6379);
	if (redisCtx == NULL || redisCtx->err) {
		if (redisCtx)
			fprintf(stderr, "Error: %s\n", redisCtx->errstr);
		exit(EXIT_FAILURE);
	}

	if (flushall)
		redisCommand(redisCtx, "FLUSHALL");

	threads = malloc(sizeof(pthread_t) * nr_threads);

	stime = time(NULL);

	for (i = 0; i < nr_threads; i++) {
		ret = pthread_create(&threads[i], NULL, redisThread, &work);
		if (ret) {
			fprintf(stderr, "Error: pthread_create %d\n", errno);
			exit(EXIT_FAILURE);
		}
	}

	for (i = 0; i < nr_threads; i++)
		pthread_join(threads[i], NULL);

	elapsed = time(NULL) - stime;
	if (!elapsed)
		elapsed = 1;

	printf("Took %lus (%lu MiB/s)\n", elapsed, (work.total_bytes >> 20) / elapsed);

	redisFree(redisCtx);
	return 0;
}

