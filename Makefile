CC=cc
CFLAGS=-Wall -O2

all: vmpte numa-thp

vmpte: vmpte-madv-dont-need.c
	$(CC) $(CFLAGS) -o vmpte vmpte-madv-dont-need.c

numa-thp: numa-file-thp.c
	$(CC) $(CFLAGS) -o numa-file-thp numa-file-thp.c

