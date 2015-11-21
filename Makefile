CC=gcc
CFLAGS=-Wall -lncurses

all: test visuals
	$(CC) $(CFLAGS) -o ncviz test.o visuals.o

visuals:
	$(CC) $(CFLAGS) -c visuals.c

test:
	$(CC) $(CFLAGS) -c test.c

clean:
	rm visuals.o test.o ncviz
