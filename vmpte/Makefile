CC=cc
CFLAGS=-Wall -O2

all: vmpte vmpte-run

vmpte: vmpte-madv-dont-need.c
	$(CC) $(CFLAGS) -o vmpte vmpte-madv-dont-need.c

vmpte-run: run-vmpte.go
	go build run-vmpte.go
clean:
	rm vmpte run-vmpte
