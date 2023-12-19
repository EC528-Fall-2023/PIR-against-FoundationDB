#include "block.h"
#include <unistd.h>
#include <string>
#include <cstring>
#include <array>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define FDB_API_VERSION 710
#include <foundationdb/fdb_c.h>

#define PORT 8080
#define PUT ((uint8_t) 1)
#define GET ((uint8_t) 2)
#define CLEAR ((uint8_t) 3)

#ifdef DEBUG
	#include <error.h>
	#define ERROR(function) error_at_line(0, errno, __FILE__, __LINE__, "%s: %s failed", __func__, function);
#else
	#define ERROR(function)
#endif

inline int setup_socket(int &server_socket, struct sockaddr_in &address, pthread_t &network_thread);

void *run_network(void *arg)
{
	if (fdb_run_network() != 0) {
		ERROR("fdb_run_network");
		exit(EXIT_FAILURE);
	}
	return arg;
}

int client_socket, server_socket;
struct FDB_database *db = NULL;
struct FDB_transaction *tr = NULL;
struct FDB_future *status = NULL;

int main()
{
	int server_socket;
	struct sockaddr_in server_addr, client_addr;

	socklen_t client_addr_len = sizeof(client_addr);

	pthread_t network_thread;
	if (setup_socket(server_socket, server_addr, network_thread) != 0) {
		ERROR("setup_socket");
		exit(EXIT_FAILURE);
	}

	while (1) {
		LISTEN:
		if ( (client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_addr_len)) < 0 ) {
			ERROR("accept");
			continue;
		}

		while (1) {
			// recv operation type
			uint8_t op;
			if (recv(client_socket, &op, sizeof(op), 0) != sizeof(op)) {
				ERROR("recv");
				goto LISTEN;
			}

			// recv key from client
			uint32_t key_size;
			if (recv(client_socket, &key_size, sizeof(key_size), 0) != sizeof(key_size)) {
				ERROR("send");
				goto LISTEN;
			}
			std::string key;
			key.resize(key_size);
			if (recv(client_socket, key.data(), key_size, MSG_WAITALL) != key_size) {
				ERROR("send");
				goto LISTEN;
			}
			std::array<uint8_t, BYTES_PER_DATA> data = {0};

			switch (op) {
			case PUT: {
				memset(data.data(), 0, BYTES_PER_DATA);
				// recv data from client
				if (recv(client_socket, data.data(), BYTES_PER_DATA, MSG_WAITALL) != BYTES_PER_DATA) {
					ERROR("send");
					goto LISTEN;
				}

				// update fdb
				if (fdb_database_create_transaction(db, &tr) != 0) {
					ERROR("fdb_database_create_transaction");
					goto LISTEN;
				}

				fdb_transaction_set(tr, (uint8_t *) key.data(), key.size(), data.data(), BYTES_PER_DATA);

				status = fdb_transaction_commit(tr);
				if ((fdb_future_block_until_ready(status)) != 0) {
					ERROR("fdb_future_block_until_ready");
					goto LISTEN;
				}
				if (fdb_future_get_error(status)) {
					ERROR("fdb_future_get_error");
					goto LISTEN;
				}
				fdb_future_destroy(status);
				fdb_transaction_destroy(tr);
				status = NULL;
				tr = NULL;

				// confirm put success
				uint8_t confirmation = 1;
				if (recv(client_socket, &confirmation, sizeof(confirmation), 0) != sizeof(confirmation)) {
					ERROR("recv");
					goto LISTEN;
				}
				if (confirmation != 1) {
					ERROR("confirmation");
					goto LISTEN;
				}
				break;
			} case GET: {
				memset(data.data(), 0, BYTES_PER_DATA);
				// get from fdb
				if (fdb_database_create_transaction(db, &tr) != 0) {
					ERROR("fdb_database_create_transaction");
					goto LISTEN;
				}

				status = fdb_transaction_get(tr, (uint8_t *) key.data(), key.size(), 0);

				if ((fdb_future_block_until_ready(status)) != 0) {
					ERROR("fdb_future_block_until_ready");
					goto LISTEN;
				}
				if (fdb_future_get_error(status)) {
					ERROR("fdb_future_get_error");
					goto LISTEN;
				}

				fdb_bool_t out_present = 0;
				const uint8_t *out_value = NULL;
				int out_value_length = 0;
				if (fdb_future_get_value(status, &out_present, &out_value, &out_value_length) == 0) {
					if (out_present) {
						memcpy(data.data(), out_value, out_value_length > (int) BYTES_PER_DATA ? BYTES_PER_DATA : out_value_length);
					} else {
					}
				} else {
					ERROR("fdb_future_get_value");
					goto LISTEN;
				}

				fdb_future_destroy(status);
				fdb_transaction_destroy(tr);
				status = NULL;
				tr = NULL;


				// recv data from server
				if (send(client_socket, data.data(), BYTES_PER_DATA, 0) != BYTES_PER_DATA) {
					ERROR("send");
					goto LISTEN;
				}

				break;
			} case CLEAR: {
				// update fdb
				if (fdb_database_create_transaction(db, &tr) != 0) {
					ERROR("fdb_database_create_transaction");
					goto LISTEN;
				}

				fdb_transaction_clear(tr, (uint8_t *) key.data(), key.size());

				status = fdb_transaction_commit(tr);
				if ((fdb_future_block_until_ready(status)) != 0) {
					ERROR("fdb_future_block_until_ready");
					goto LISTEN;
				}
				if (fdb_future_get_error(status)) {
					ERROR("fdb_future_get_error");
					goto LISTEN;
				}
				fdb_future_destroy(status);
				fdb_transaction_destroy(tr);
				status = NULL;
				tr = NULL;

				// confirm put success
				uint8_t confirmation = 1;
				if (recv(client_socket, &confirmation, sizeof(confirmation), 0) != sizeof(confirmation)) {
					ERROR("recv");
					goto LISTEN;
				}
				if (confirmation != 1) {
					ERROR("confirmation");
					goto LISTEN;
				}
				break;

			} default:
				shutdown(server_socket, SHUT_RDWR);
				if (fdb_stop_network() != 0) {
					ERROR("fdb_stop_network");
					exit(EXIT_FAILURE);
				}
				pthread_join(network_thread, NULL);
				fdb_database_destroy(db);

				exit(0);
			}
		}
	}

	shutdown(server_socket, SHUT_RDWR);

	if (fdb_stop_network() != 0) {
		ERROR("fdb_stop_network");
		exit(EXIT_FAILURE);
	}
	pthread_join(network_thread, NULL);
	fdb_database_destroy(db);

	return 0;
}

inline int setup_socket(int &server_socket, struct sockaddr_in &address, pthread_t &network_thread)
{
	if ( (server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		ERROR("socket");
		return -1;
	}

	int opt = 1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		ERROR("setsockopt");
		close(server_socket);
		return -1;
	}

	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);
	
	if (bind(server_socket, (struct sockaddr *) &address, sizeof(address)) < 0) {
		ERROR("bind");
		close(server_socket);
		return -1;
	}

	if (listen(server_socket, 3) < 0) {
		ERROR("listen");
		close(server_socket);
		return -1;
	}

	fdb_select_api_version(FDB_API_VERSION);

	if (fdb_setup_network() != 0) {
		ERROR("fdb_setup_network");
		return -1;
	}

	if (pthread_create(&network_thread, NULL, run_network, NULL) != 0) {
		ERROR("pthread_create");
		return -1;
	}

	if (fdb_create_database(NULL, &db) != 0) {
		ERROR("fdb_create_database");
		return -1;
	}

	return 0;
}

