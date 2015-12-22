CC=gcc
CFLAGS=-Wall -lncurses

all: example ncviz
	$(CC) $(CFLAGS) -o example example.o ncviz.o

ncviz: ncviz.c
	$(CC) $(CFLAGS) -c ncviz.c

example: example.c
	$(CC) $(CFLAGS) -c example.c

clean:
	rm ncviz.o example.o example
