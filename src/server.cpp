#include "tree.h"
#include <iostream>
#include <string>
#include <stdlib.h>
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
		std::cerr << "fdb_run_network: failed\n";
		exit(EXIT_FAILURE);
	}
	return arg;
}

int main()
{
	int server_socket;
	if ( (server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		std::cerr << "socket: failed\n";
		exit(EXIT_FAILURE);
	}

	int opt = 1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		std::cerr << "setsockopt: failed\n";
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	if (bind(server_socket, (struct sockaddr *) &address, sizeof(address)) < 0) {
		std::cerr << "bind: failed\n";
		exit(EXIT_FAILURE);
	}

	if (listen(server_socket, 3) < 0) {
		std::cerr << "listen: failed\n";
		exit(EXIT_FAILURE);
	}

	socklen_t addr_len = sizeof(address);
	int client_socket;

	fdb_select_api_version(FDB_API_VERSION);
	if (fdb_setup_network() != 0) {
		std::cerr << "fdb_setup_network: failed\n";
		exit(EXIT_FAILURE);
	}
	pthread_t network_thread;
	if (pthread_create(&network_thread, NULL, run_network, NULL) != 0) {
		std::cerr << "pthread_create: failed\n";
		exit(EXIT_FAILURE);
	}

	struct FDB_database *db = NULL;
	if (fdb_create_database(NULL, &db) != 0) {
		std::cerr << "fdb_create_database: failed\n";
		exit(EXIT_FAILURE);
	}

	while (1) {
		LISTEN:
		if ( (client_socket = accept(server_socket, (struct sockaddr *) &address, &addr_len)) < 0 ) {
			std::cerr << "accept: failed\n";
			exit(EXIT_FAILURE);
		}
		std::cout << "server: connection established\n";

		while (1) {
			struct FDB_transaction *tr = NULL;
			struct FDB_future *status = NULL;

			uint32_t leaf_id;
			switch (recv(client_socket, &leaf_id, 4, 0)) {
			case 0:
				if (status != NULL)
					fdb_future_destroy(status);
				if (tr != NULL)
					fdb_transaction_destroy(tr);
				std::cout << "server: client disconnected\n";
				goto LISTEN;
				break;
			case 4:
				break;
			default:
				std::cerr << "recv: failed\n";
				exit(EXIT_FAILURE);
			}
			std::cout << "server: leaf_id = " << leaf_id << '\n';

			// TODO: find actual num_buckets when traversing the tree
			uint32_t num_buckets = 2;
			if (send(client_socket, &num_buckets, 4, 0) != 4) {
				std::cerr << "send: failed\n";
				exit(EXIT_FAILURE);
			}
			std::cout << "server: sent num_buckets\n";

			std::vector<uint32_t> leaf_ids (2 * BLOCKS_PER_BUCKET);
			if (send(client_socket, leaf_ids.data(), 8 * BLOCKS_PER_BUCKET, 0) != 8 * BLOCKS_PER_BUCKET) {
				std::cerr << "send: failed\n";
				exit(EXIT_FAILURE);
			}
			std::cout << "server: sent leaf_ids\n";

			std::array<uint8_t, BYTES_PER_BLOCK> data_buffer;
			data_buffer.fill(1);
			for (uint32_t i = 0; i < num_buckets * BLOCKS_PER_BUCKET; ++i) {
				if (send(client_socket, data_buffer.data(), BYTES_PER_BLOCK, 0) != BYTES_PER_BLOCK) {
					std::cerr << "send: failed\n";
					exit(EXIT_FAILURE);
				}
			}
			std::cout << "server: sent 6 blocks (2 buckets)\n";

			if (fdb_database_create_transaction(db, &tr) != 0) {
				std::cerr << "fdb_database_create_transaction: failed\n";
				exit(EXIT_FAILURE);
			}

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
