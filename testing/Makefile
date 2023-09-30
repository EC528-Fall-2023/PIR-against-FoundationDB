CC=gcc
FLAGS=-std=c11 -g -lm -lpthread -lrt

all: src/test.c
	$(CC) $(FLAGS) src/test.c /usr/lib64/libfdb_c.so -o test

socket: src/client.c
	$(CC) $(FLAGS) src/server.c -o server
	$(CC) $(FLAGS) src/client.c -o client

clean:
	rm test client server
