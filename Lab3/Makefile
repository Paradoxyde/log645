PROG = lab3
CFLAGS = -g -fPIC -m64 -Wall -std=c99
LFLAGS= -fPIC -m64 -Wall
CC = mpicc

all: $(PROG)

lab3.o: lab3.c
	$(CC) $(CFLAGS) -c lab3.c

lab3 : lab3.o
	$(CC) $(LFLAGS) lab3.o -o lab3

run:
	mpirun --hostfile hostfile -np 24 lab3

ps:
	ps -fu $$USER

clean:
	/bin/rm -f lab3
	/bin/rm -f lab3.o