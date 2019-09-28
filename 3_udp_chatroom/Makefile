CC=gcc
CFLAGS=-g -Wall -O2 -c

all:server client

server:server.o
	$(CC) $+ -o $@

client:client.o
	$(CC) $+ -o $@ -lpthread

%*.o:%*.c
	$(CC) $(CFLAGS) $< -o $@ -lpthread

clean:
	$(RM) *.o client server
