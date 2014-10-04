PROG = tp1
CFLAGS = -g -fPIC -m64 -Wall -std=c99 -D_BSD_SOURCE -D_POSIX_C_SOURCE=200112
LFLAGS= -fPIC -m64 -Wall -Wl,--no-as-needed -lrt
CC = mpicc

all: $(PROG)

tp1.o: tp1.c
	$(CC) $(CFLAGS) -c tp1.c

tp1 : tp1.o
	$(CC) $(LFLAGS) tp1.o -o tp1

run:
	mpirun --hostfile hostfile -np 23 tp1

ps:
	ps -fu $$USER

clean:
	/bin/rm -f tp1
	/bin/rm -f tp1.o