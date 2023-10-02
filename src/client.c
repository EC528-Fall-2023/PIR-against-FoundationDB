#include "../include/client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 8080

static bool initialized = false;
static int socket_fd;

enum operation {
	READ = 1,
	WRITE,
	READ_RANGE,
	CLEAR_RANGE
};

static void initialize()
{
	if ( (socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		printf("socket: failed\n");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);

	if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
		printf("inet_pton: failed\n");
		exit(EXIT_FAILURE);
	}

	int status;
	if ( (status = connect(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr))) < 0 ) {
		printf("connect: failed\n");
		exit(EXIT_FAILURE);
	}
	initialized = true;
}

static void send_data(uint8_t op, uint8_t id, const uint8_t *data, uint16_t data_length)
{
	if (!initialized)
		initialize();
	// request => op (1 byte) + id (1 byte) + data_length (2 bytes) + data (data_length bytes)
	uint8_t request[4096] = {0};
	request[0] = op;
	request[1] = id;
	memcpy(request+2, &data_length, 2);
	memcpy(request+4, data, data_length);
	send(socket_fd, request, 4 + data_length, 0);
	printf("Client: Hello message sent\n");
}

uint8_t get(uint8_t const *key_name, int key_name_length)
{
	key_name = key_name;
	key_name_length = key_name_length;
	send_data(READ, 1, NULL, 0);
	return 0;
}

void put(uint8_t const *key_name, int key_name_length, uint8_t const *value, int value_length)
{
	key_name = key_name;
	key_name_length = key_name_length;
	send_data(WRITE, 1, value, value_length);
}

/*
int main()
{
	int socket_fd;
	if ( (socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		printf("socket: failed\n");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);

	if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
		printf("inet_pton: failed\n");
		exit(EXIT_FAILURE);
	}

	int status;
	if ( (status = connect(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr))) < 0 ) {
		printf("connect: failed\n");
		exit(EXIT_FAILURE);
	}

	

	return 0;
}
*/
