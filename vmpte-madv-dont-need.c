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
	size_t map_size = TiB;
	void *mem;
	char *p, *end, *estr;
	int opt, wait_us = 0;
	int ret;

	while ((opt = getopt(argc, argv, "mw:") != -1)) {
		switch (opt) {
		case 'w':
			wait_us = atoi(optarg);
			break;
		case 'm':
			map_size = strtoul(optarg, NULL, 10);
			map_size = map_size << 30;
		default:
			exit(EXIT_FAILURE);
		}
	}

	printf("Allocating %lu GiB virt\nSleeping for %d ms\n",
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

	return 0;
failed:
	perror(estr);
	fprintf(stderr, "Please enable memory overcommit, or use vmpte.sh script!\n");
	exit(EXIT_FAILURE);
}
