CC = gcc
CFLAGS = -Wall
BIN_DIR = bin

all: $(BIN_DIR) $(BIN_DIR)/client $(BIN_DIR)/server

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

commom.o: commom.c
	$(CC) $(CFLAGS) -c commom.c -o commom.o

$(BIN_DIR)/client: client.c commom.o
	$(CC) $(CFLAGS) client.c commom.o -o $(BIN_DIR)/client -lm

$(BIN_DIR)/server: server.c commom.o
	$(CC) $(CFLAGS) server.c commom.o -o $(BIN_DIR)/server -lpthread

clean:
	rm -f *.o
	rm -rf $(BIN_DIR)
