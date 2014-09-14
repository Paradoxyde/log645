PROG = main
CFLAGS = -g -fPIC -m64 -Wall -std=c99 -D_BSD_SOURCE -D_POSIX_C_SOURCE=200112
LFLAGS= -fPIC -m64 -Wall -Wl,--no-as-needed -lrt
CC = gcc

all: $(PROG)

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

main : main.o
	$(CC) $(LFLAGS) main.o -o main

run:
	./main

ps:
	ps -fu $$USER

clean:
	/bin/rm -f *~
	/bin/rm -f *.o