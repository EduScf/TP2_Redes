CC = gcc
CFLAGS = -Wall

all: client server server-mt

commom.o: commom.c
	$(CC) $(CFLAGS) -c commom.c -o commom.o

client: client.c commom.o
	$(CC) $(CFLAGS) client.c commom.o -o client -lm

server: server.c commom.o
	$(CC) $(CFLAGS) server.c commom.o -o server

server-mt: server-mt.c commom.o
	$(CC) $(CFLAGS) server-mt.c commom.o -o server-mt -lpthread

clean:
	rm -f *.o client server server-mt