CC=gcc
CFLAGS = -Wall -lncurses

all: visuals.o
	$(CC) $(CFLAGS) -o nzviz visuals.o

visuals.o:
	$(CC) $(CFLAGS) -c visuals.c

clean:
	rm -f ppviz
