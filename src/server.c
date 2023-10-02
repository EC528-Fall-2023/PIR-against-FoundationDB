#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080

#define FDB_API_VERSION 710
#include <foundationdb/fdb_c.h>

struct data_block {
	uint16_t id;
	uint16_t data_length;
	uint8_t *data;
};

int main()
{
	int server_socket;
	if ( (server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		printf("socket: failed\n");
		exit(EXIT_FAILURE);
	}

	int opt = 1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		printf("setsockopt: failed\n");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	if (bind(server_socket, (struct sockaddr *) &address, sizeof(address)) < 0) {
		printf("bind: failed\n");
		exit(EXIT_FAILURE);
	}

	if (listen(server_socket, 3) < 0) {
		printf("listen: failed\n");
		exit(EXIT_FAILURE);
	}

	socklen_t addr_len = sizeof(address);
	int client_socket;
	if ( (client_socket = accept(server_socket, (struct sockaddr *) &address, &addr_len)) < 0 ) {
		printf("accept: failed\n");
		exit(EXIT_FAILURE);
	}

	long page_size = sysconf(_SC_PAGESIZE);
	uint8_t *data = (uint8_t *) malloc(page_size);
	while (1) {
	// data_request => op (1 byte) + id (1 byte) + data_length (2 bytes) + data (data_length bytes)
		uint8_t op;
		uint8_t id;
		uint16_t data_length;
		memset(data, 0, page_size);
		if (read(client_socket, &op, sizeof(uint8_t)) != sizeof(uint8_t)) {
			printf("read: op failed\n");
			break;
		}
		printf("op: %x\n", op);
		if (op == 0)
			break;
		if (read(client_socket, &id, sizeof(uint8_t)) != sizeof(uint8_t)) {
			printf("read: id failed\n");
			break;
		}
		printf("id: %x\n", id);
		if (read(client_socket, &data_length, sizeof(uint16_t)) != sizeof(uint16_t)) {
			printf("read: data_length failed\n");
			break;
		}
		printf("data_length: %x\n", data_length);
		if (read(client_socket, data, data_length) != data_length) {
			printf("read: data failed\n");
			break;
		}
		printf("Server: message received: %s\n", data);
	}

	close(client_socket);
	shutdown(server_socket, SHUT_RDWR);

	return 0;
}
