#include <sys/mman.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define MB (1UL << 20)
#define GB (1UL << 30)
#define TB (1UL << 40)
#define THP (MB * 2)
#define PAGE (1 << 12)

int main(int argc, char **argv)
{
	void *p;
	char *page;
	size_t mmap_len = 4;
	size_t nr_thps;
	int stime = 3600;
	int ret;

	if (argc > 1)
		mmap_len = atoi(argv[1]);

	if (argc > 2)
		stime = atoi(argv[2]);

	mmap_len *= GB;

	p = mmap(0, mmap_len, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	if (p == MAP_FAILED) {
		fprintf(stderr, "mmap failed %d", errno);
		exit(1);
	}

	printf("%lu GiB, %d seconds\n", mmap_len >> 30, stime);

	// printf("allocated %lu MiB at %p\n", mmap_len >> 20, p);

	ret = madvise(p, mmap_len, MADV_NOHUGEPAGE);
	if (ret) {
		fprintf(stderr, "madvise failed %d\n", errno);
		exit(EXIT_FAILURE);
	}

	nr_thps = mmap_len / THP;
	for (int i = 0; i < nr_thps * 2; i++) {
		int pi = random() % nr_thps;
		page = p + pi * THP;
		*((volatile char *)page) = 0;
		madvise(page, PAGE, MADV_DONTNEED);
		usleep(random() % 97);
	}

	// printf("sleeping for %d seconds\n", stime);
	sleep(stime);
}
