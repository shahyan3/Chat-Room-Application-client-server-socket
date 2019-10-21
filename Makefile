# Makefile for fibfork

CC = gcc

CFLAGS = -Wall

all: server
	echo "Done."

server1.o: server_functions.h 


server: server1.o
 
 
# fibfork: fibfork.o fib.o

clean:
	rm -f server
	rm -f *.o

.PHONY: all clean 