CC=cc
CFLAGS=-Wall -O2

all: numa-thp

numa-thp: numa-file-thp.c
	$(CC) $(CFLAGS) -o numa-file-thp numa-file-thp.c

