CC=gcc

all: bsh

bsh: bsh.c
	$(CC) -g -o bsh bsh.c

clean:
	rm -f bsh a.out
