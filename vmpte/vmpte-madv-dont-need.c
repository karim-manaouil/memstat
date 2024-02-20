/* GPL v2 software
 *
 * Author: Karim Manaouil <k.manaouil@gmail.com>
 *		Edinburgh University 23/24
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#define PROT_COMMIT (PROT_READ | PROT_WRITE)

#define THP	(2UL << 20)
#define GiB	(1UL << 30)
#define TiB	(1UL << 40)

#define touch(p) (*(char volatile *)p = 0xff)

int main(int argc, char **argv)
{
	char cmd_fmt[64];
	size_t map_size = TiB;
	void *mem;
	char *p, *end, *estr;
	int opt, sleep_s = 0, wait_us = 0;
	int pid, ret;

	while ((opt = getopt(argc, argv, "d:m:s:")) != -1) {
		switch (opt) {
		case 'd':
			wait_us = atoi(optarg);
			break;
		case 's':
			sleep_s = atoi(optarg);
			break;
		case 'm':
			map_size = strtoul(optarg, NULL, 10);
			map_size = map_size << 30;
			break;
		default:
			char *usage_fmt = "Usage: %s [-p usecs] [-m GiBs]\n";
			fprintf(stderr, usage_fmt, argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	printf("Allocating %lu GiB virt\nPausing for %d ms\n",
			map_size == TiB? 1024: map_size >> 30, wait_us);

	mem = mmap(0, map_size, PROT_COMMIT, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (mem == MAP_FAILED) {
		estr = "mmap";
		goto failed;
	}

	ret = madvise(mem, map_size, MADV_NOHUGEPAGE);
	if (ret) {
		estr = "madvise";
		goto failed;
	}

	p = mem;
	end = p + map_size;

	while (p < end) {
		touch(p);
		ret = madvise((void *)p, THP, MADV_DONTNEED);
		if (ret) {
			estr = "madvise";
			goto failed;
		}
		p += THP;
		if (wait_us) {
			ret = usleep(wait_us);
			if (ret) {
				perror("usleep");
				exit(EXIT_FAILURE);
			}
		}
	}
	pid = getpid();
	sprintf(cmd_fmt, "cp /proc/%d/status /tmp/vmpte", pid);
	ret = system(cmd_fmt);
	if (ret) {
		fprintf(stderr, "Could not write status file\n");
		exit(1);
	}

	printf("Waiting for %d secs before exiting...\n", sleep_s);
	sleep(sleep_s);
	return 0;
failed:
	perror(estr);
	fprintf(stderr, "Please enable memory overcommit, or use vmpte.sh script!\n");
	exit(EXIT_FAILURE);
}
