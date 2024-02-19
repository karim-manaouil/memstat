#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>

#define MEM_LEN (2 << 20)

void *memory;

int main(int argc, char **argv)
{
	char volatile *p;
	int p_aff = 0, c_aff = 0;
	int opt;
	int pid;
	int ret;

	while ((opt = getopt(argc, argv, "pc:") != -1)) {
		switch (opt) {
		case 'p':
			p_aff = atoi(optarg);
			printf("Parent pinned on %d\n", p_aff);
			break;
		case 'c':
			c_aff = atoi(optarg);
			printf("Child pinned on %d\n", c_aff);
			break;
		default:
			exit(EXIT_FAILURE);
		}
	}

	memory = mmap(0, MEM_LEN, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
	if (!memory) {
		perror("mmap:");
		exit(EXIT_FAILURE);
	}

	ret = madvise(memory, MEM_LEN, MADV_HUGEPAGE);
	if (ret == -1) {
		perror("madvise:");
		exit(EXIT_FAILURE);
	}

	pid = fork();
	if (pid) {
		ret = wait(NULL);
		if (ret == -1) {
			perror("wait:");
			exit(EXIT_FAILURE);
		}
		exit(1);
	}

	p = memory;
	while (1) {
		int rnd = rand();
		p[rnd % MEM_LEN] = (char)rnd;
	}
}
