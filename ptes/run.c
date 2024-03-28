#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

#define program "/root/ptes/mmap"

#define MAP_SIZE 256
#define SHORT_WAIT 0
#define LONG_WAIT 3600

int max = 120;
int longs = 20;
int shorts = 80;

void run(int memory, int stime)
{
	int pid, ret;
	char arg1[16], arg2[16];
	char *argv[] = {"mmap", arg1, arg2, 0};

	sprintf(arg1, "%d", memory);
	sprintf(arg2, "%d", stime);

	pid = fork();
	if (!pid) {
		ret = execve(program, argv, 0);
		if (ret == -1) {
			fprintf(stderr, "execve failed %d: %s\n", errno, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
	usleep(1000*300);
}

void run_long(void) {
	return run(MAP_SIZE, LONG_WAIT);
}

void run_short(void) {
	return run(MAP_SIZE, SHORT_WAIT);
}

long biased_random(void)
{
	long rand;

	rand = random() % max;
	if (rand <= shorts)
		return 0;
	else
		return 1;
}

int main(int argc, char **argv)
{
	int nr_l = 0, nr_s = 0;
	long rand;
	int status, r;

	if (argc > 1)
		max = atoi(argv[1]);

	longs	= max * longs / 100;
	shorts	= max * shorts / 100;

	while ((nr_l + nr_s) < max) {
		rand = biased_random();
try_long:
		if (rand) {
			if (nr_l < longs) {
				run_long();
				nr_l++;
			} else {
				if ((nr_l + nr_s) < max)
					rand = 0;
			}
		}
		if (!rand) {
			if (nr_s < shorts) {
				run_short();
				nr_s++;
			} else {
				if ((nr_l + nr_s) < max) {
					rand = 1;
					goto try_long;
				}
			}
		}
	}

	while (1) {
		r = wait(&status);
		if (r == -1)
		       break;
		if (WEXITSTATUS(status) != 0) {
			fprintf(stderr, "A child failed %d\n", errno);
			exit(EXIT_FAILURE);
		}
	}
}

