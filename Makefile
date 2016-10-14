CC = gcc
CFLAGS = -g
all:: test

test: test.o
	$(CC) -w -o test test.o thread.o
test.o: thread
	$(CC) -g -w -c test.c -o test.o
thread:
	$(CC) -g -w -c thread.c -o thread.o
clean:
	rm *.o
