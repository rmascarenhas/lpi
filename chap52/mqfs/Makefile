CC = cc
CFLAGS = -g -O2  -Wall -Wextra

LIBS = -lrt
OBJ = common.o
CLIENT = client
SERVER = server

all: client server
common.o: common.h common.c

client: client.c common.o
	$(CC) -o $(CLIENT) $(CFLAGS) client.c $(OBJ) $(LIBS)

server: server.c common.o
	$(CC) -o $(SERVER) $(CFLAGS) server.c $(OBJ) $(LIBS)

clean:
	rm -fv *.o $(CLIENT) $(SERVER)
