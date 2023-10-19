#include "tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080

#define FDB_API_VERSION 710
#include <foundationdb/fdb_c.h>

enum operation {
	READ = 1,
	WRITE,
	READ_RANGE,
	CLEAR_RANGE
};

void *run_network(void *arg)
{
	if (fdb_run_network() != 0) {
		printf("fdb_run_network: failed\n");
		exit(EXIT_FAILURE);
	}
	return NULL;
}

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

	fdb_select_api_version(FDB_API_VERSION);
	if (fdb_setup_network() != 0) {
		printf("fdb_setup_network: failed\n");
		exit(EXIT_FAILURE);
	}
	pthread_t network_thread;
	if (pthread_create(&network_thread, NULL, run_network, NULL) != 0) {
		printf("pthread_create: failed\n");
		exit(EXIT_FAILURE);
	}

	struct FDB_database *db = NULL;
	if (fdb_create_database(NULL, &db) != 0) {
		printf("fdb_create_database: failed\n");
		exit(EXIT_FAILURE);
	}

	while (1) {
		if ( (client_socket = accept(server_socket, (struct sockaddr *) &address, &addr_len)) < 0 ) {
			printf("accept: failed\n");
			exit(EXIT_FAILURE);
		}

		while (1) {
			std::array<uint8_t, BYTES_PER_BLOCK> data_buffer;
			data_buffer.fill(0);
			if (recv(client_socket, data_buffer.data(), BYTES_PER_BLOCK, 0) != BYTES_PER_BLOCK) {
				printf("recv: failed\n");
				exit(EXIT_FAILURE);
			}

			struct FDB_transaction *tr = NULL;
			if (fdb_database_create_transaction(db, &tr) != 0) {
				printf("fdb_database_create_transaction: failed\n");
				exit(EXIT_FAILURE);
			}

			struct FDB_future *status = NULL;
			// do stuff with status

			if (status != NULL)
				fdb_future_destroy(status);
			if (tr != NULL)
				fdb_transaction_destroy(tr);
		}
	}

	close(client_socket);
	shutdown(server_socket, SHUT_RDWR);

	return 0;
}
