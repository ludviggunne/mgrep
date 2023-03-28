CC=gcc
CFLAGS=-g

mgrep: main.o
	$(CC) $(CFLAGS) main.o -o mgrep

main.o: main.c
	$(CC) -c $(CFLAGS) main.c -o main.o

.PHONY:
	clean

clean:
	rm -f main.o mgrep