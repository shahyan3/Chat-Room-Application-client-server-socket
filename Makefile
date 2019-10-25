# # Makefile for fibfork

# CC = gcc

# CFLAGS = -Wall

# all: server
# 	echo "Done."

# server1.o: server_functions.h 


# server: server1.o
 
 
# # fibfork: fibfork.o fib.o

# clean:
# 	rm -f server
# 	rm -f *.o

# .PHONY: all clean 



TARGET=client server  

CC=gcc

CFLAGS= -Wall -Wextra -g -I. -pthread

normal: $(TARGET)

server: server1.c
	$(CC) $(CFLAGS) server1.c -o server

client: client1.c
	$(CC) $(CFLAGS) client1.c -o client

clean:
	$(RM) $(TARGET)