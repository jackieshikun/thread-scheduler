CC = gcc
CFLAGS = -g
all:: test

test: thread
	$(CC) -w -o test thread.o
thread:
	$(CC) -g -w -c thread.c -o thread.o
clean:
	rm *.o
