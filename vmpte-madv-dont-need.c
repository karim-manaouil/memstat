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

int main()
{
	size_t map_size = TiB;
	void *mem;
	char *p, *end;
	int ret;

	mem = mmap(0, map_size, PROT_COMMIT, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (mem == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}

	ret = madvise(mem, map_size, MADV_NOHUGEPAGE);
	if (ret)
		goto madv_failed;

	p = mem;
	end = p + map_size;

	while (p < end) {
		touch(p);
		ret = madvise((void *)p, THP, MADV_DONTNEED);
		if (ret)
			goto madv_failed;
		p += THP;
	}

	return 0;

madv_failed:
	perror("madvise");
	exit(EXIT_FAILURE);
}
