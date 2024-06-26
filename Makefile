CC=cc
CFLAGS=-Wall -O2

all: numa-thp

numa-thp: numa-file-thp.c
	$(CC) $(CFLAGS) -o numa-file-thp numa-file-thp.c

perf: perf.c
	$(CC) $(CFLAGS) -o perf perf.c
redis-bench: redis-bench.c
	$(CC) $(CFLAGS) redis-bench.c -o redis-bench -lhiredis
