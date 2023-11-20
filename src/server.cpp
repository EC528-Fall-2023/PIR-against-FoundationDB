#include "block.h"
#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <random>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define PORT 8080

#define FDB_API_VERSION 710
#include <foundationdb/fdb_c.h>

void printBinaryBuffer(const uint8_t* buffer, size_t size) {
	for (size_t i = 0; i < size; ++i) {
		printf("\\x%02X", buffer[i]);
	}
}
inline int setup_socket(int &server_socket, struct sockaddr_in &address, pthread_t &network_thread);
inline int setup_fdb();
inline int get_branch_indexes(std::vector<uint16_t> &branch_indexes, uint16_t leaf_id);
inline int get_branch_from_fdb(std::vector<Block> &branch, std::vector<uint16_t> &branch_indexes);
inline int send_branch_to_client(std::vector<Block> &branch);
inline int receive_updated_blocks(std::vector<Block> &branch);
inline int send_branch_to_fdb(std::vector<Block> &branch, std::vector<uint16_t> &branch_indexes);
inline void close_connection();

void *run_network(void *arg)
{
	if (fdb_run_network() != 0) {
		std::cerr << "fdb_run_network: failed\n";
		exit(EXIT_FAILURE);
	}
	return arg;
}

int client_socket;
struct FDB_database *db = NULL;
struct FDB_transaction *tr = NULL;
struct FDB_future *status = NULL;
static std::random_device generator;
static std::uniform_int_distribution<uint8_t> random_byte(0, 0xff);
uint8_t fdb_is_initialized = 0;

uint8_t *enc_key = (uint8_t *) "0123456789abcdeF0123456789abcdeF";
uint8_t *enc_iv = (uint8_t *) "1234567890abcdef";

int main()
{
	int server_socket;
	struct sockaddr_in server_addr, client_addr;

	socklen_t client_addr_len = sizeof(client_addr);

	pthread_t network_thread;
	if (setup_socket(server_socket, server_addr, network_thread) != 0) {
		std::cerr << "setup_socket: failed\n";
		exit(EXIT_FAILURE);
	}

	while (1) {
		LISTEN:
		if ( (client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_addr_len)) < 0 ) {
			std::cerr << "server: accept: failed\n";
			continue;
		}
		char client_ip[INET_ADDRSTRLEN] = {0};
		inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
#ifdef DEBUG
		std::cout << "server: connection to " << client_ip << ':' << PORT << " established\n";
#endif

		if (setup_fdb() != 0) {
			std::cerr << "setup_fdb: failed\n";
			exit(EXIT_FAILURE);
		}

		while (1) {
			// receive request_id
			uint16_t request_id;
			switch (recv(client_socket, &request_id, sizeof(request_id), 0)) {
			case 0:
#ifdef DEBUG
				std::cout << "server: client disconnected\n";
#endif
				goto LISTEN;
			case sizeof(request_id):
				break;
			default:
				perror("server: recv: failed");
				close_connection();
				goto LISTEN;
			}
#ifdef DEBUG
			std::cout << "server: request_id = " << request_id << '\n';
#endif

			// receive leaf_id
			uint16_t leaf_id;
			if (recv(client_socket, &leaf_id, sizeof(leaf_id), 0) != sizeof(leaf_id)) {
				perror("server: recv: failed");
				close_connection();
				continue;
			}
#ifdef DEBUG
			std::cout << "server: leaf_id = " << leaf_id << '\n';
#endif

			// contains indexes that correspond to nodes on the branch
			std::vector<uint16_t> branch_indexes;
			if (get_branch_indexes(branch_indexes, leaf_id) != 0) {
				std::cerr << "server: get_branch_indexes: failed";
				close_connection();
				continue;
			}

			// send number of blocks to send
			uint16_t num_blocks = branch_indexes.size() * BLOCKS_PER_BUCKET;
			if (send(client_socket, &num_blocks, sizeof(num_blocks), 0) != sizeof(num_blocks)) {
				perror("server: send: failed");
				close_connection();
				continue;
			}
#ifdef DEBUG
			std::cout << "server: sent num_blocks = " << num_blocks << '\n';
#endif

			// retrieve vector<Block> from fdb
			std::vector<Block> branch(num_blocks);
			if (get_branch_from_fdb(branch, branch_indexes) != 0) {
				std::cerr << "server: get_branch_from fdb\n";
				close_connection();
				continue;
			}
#ifdef DEBUG
			std::cout << "server: retrieved branch from fdb\n";
#endif

			if (send_branch_to_client(branch) != 0) {
				perror("server: send_branch_to_client: failed");
				close_connection();
				continue;
			}

			// receive updated blocks from client
			if (receive_updated_blocks(branch) != 0) {
				std::cerr << "server: receive_updated_blocks: failed\n";
				close_connection();
				continue;
			}

			// send updated blocks to fdb
			if (send_branch_to_fdb(branch, branch_indexes) != 0) {
				perror("server: send_branch_to_fdb: failed");
				close_connection();
				continue;
			}

			// send acknowledgement of successful operation
			if (send(client_socket, &request_id, sizeof(request_id), 0) != sizeof(request_id)) {
				perror("server: send: failed acknowledgement");
				close_connection();
				continue;
			}

			// print results
#ifdef DEBUG
			std::cout << "server: showing " << num_blocks << " blocks, their leaf_ids and first 10 bytes of the data\n";
			for (unsigned long i = 0; i < branch.size(); ++i) {
				if (i % 3 == 0)
					std::cout << "---------------------------\n";
				std::cout << branch[i].get_block_id() << ',' << '\t';
				for (int j = 0; j < 10; ++j) {
					char c = branch[i].get_decrypted_data()[j];
					if (c < 32 || c > 126)
						c = '.';
					std::cout << c << ' ';
				}
				std::cout << '\n';
			}
#endif
		}
	}

	shutdown(server_socket, SHUT_RDWR);

	if (fdb_stop_network() != 0) {
		std::cerr << "server: fdb_stop_network: failed\n";
		exit(EXIT_FAILURE);
	}
	pthread_join(network_thread, NULL);
	fdb_database_destroy(db);

	return 0;
}

inline int setup_socket(int &server_socket, struct sockaddr_in &address, pthread_t &network_thread)
{
	if ( (server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		std::cerr << "server: socket: failed\n";
		return -1;
	}

	int opt = 1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		std::cerr << "server: setsockopt: failed\n";
		close(server_socket);
		return -1;
	}

	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);
	
	if (bind(server_socket, (struct sockaddr *) &address, sizeof(address)) < 0) {
		std::cerr << "server: bind: failed\n";
		close(server_socket);
		return -1;
	}

	if (listen(server_socket, 3) < 0) {
		std::cerr << "server: listen: failed\n";
		close(server_socket);
		return -1;
	}

	fdb_select_api_version(FDB_API_VERSION);

	if (fdb_setup_network() != 0) {
		std::cerr << "fdb_setup_network: failed\n";
		return -1;
	}

	if (pthread_create(&network_thread, NULL, run_network, NULL) != 0) {
		std::cerr << "pthread_create: failed\n";
		return -1;
	}

	if (fdb_create_database(NULL, &db) != 0) {
		std::cerr << "fdb_create_database: failed\n";
		return -1;
	}

	return 0;
}

inline int setup_fdb()
{
	// check if tree was initiated already by checking if key: \x01 = value: \x01
	if (fdb_database_create_transaction(db, &tr) != 0) {
		std::cerr << "fdb_database_create_transaction: failed\n";
		return -1;
	}

	uint8_t key = 1;
	status = fdb_transaction_get(tr, &key, sizeof(key), 0);

	if ((fdb_future_block_until_ready(status)) != 0) {
		std::cerr << "fdb_future_block_until_ready: failed\n";
		return -1;
	}
	if (fdb_future_get_error(status)) {
		std::cerr << "fdb_future_is_error: its an error...\n";
		return -1;
	}

	fdb_bool_t out_present = 0;
	const uint8_t *out_value = NULL;
	int out_value_length = 0;
	if (fdb_future_get_value(status, &out_present, &out_value, &out_value_length) == 0) {
		if (out_present == 0 || out_value[0] == 0)
			fdb_is_initialized = 0;
		else
			fdb_is_initialized = 1;
		if (send(client_socket, &fdb_is_initialized, sizeof(fdb_is_initialized), 0) != sizeof(fdb_is_initialized)) {
			perror("send: failed");
			return -1;
		}
	} else {
		std::cerr << "fdb_future_get_value: failed\n";
		return -1;
	}
	fdb_future_destroy(status);
	fdb_transaction_destroy(tr);
	status = NULL;
	tr = NULL;

	if (!fdb_is_initialized) {
		tr = NULL;
		if (fdb_database_create_transaction(db, &tr) != 0) {
			std::cerr << "fdb_database_create_transaction: failed\n";
			return -1;
		}

		for (uint16_t tree_index = 1; tree_index != 0; ++tree_index) {
			uint8_t temp[3] = {0};
			temp[1] = (tree_index & 0xff00) >> 8;
			temp[2] = tree_index & 0x00ff;
			uint8_t bucket[BLOCK_SIZE * BLOCKS_PER_BUCKET];

			for (int block_idx = 0; block_idx < BLOCKS_PER_BUCKET; ++block_idx) {
				Block block;
				if (recv(client_socket, block.get_encrypted_data(), BLOCK_SIZE, MSG_WAITALL) != BLOCK_SIZE) {
					perror("server: recv");
					return -1;
				}
				memcpy(bucket + BLOCK_SIZE * block_idx, block.get_encrypted_data(), BLOCK_SIZE);
			}

			fdb_transaction_set(tr, (const uint8_t *) temp, sizeof(temp), bucket, BLOCK_SIZE * BLOCKS_PER_BUCKET);

			if (tree_index % 3000 == 0) {
				status = fdb_transaction_commit(tr);
				if ((fdb_future_block_until_ready(status)) != 0) {
					std::cerr << "fdb_future_block_until_ready: failed\n";
					return -1;
				}
				int error = fdb_future_get_error(status);
				if (error != 0) {
					std::cerr << "fdb_future_get_error: " << error << '\n';
					return -1;
				}
				fdb_future_destroy(status);
				fdb_transaction_destroy(tr);
				status = NULL;
				tr = NULL;
				if (fdb_database_create_transaction(db, &tr) != 0) {
					std::cerr << "fdb_database_create_transaction: failed\n";
					return -1;
				}

#ifdef DEBUG
				std::cout << "server: initialized " << tree_index << " buckets\n";
#endif
			}
		}

		status = fdb_transaction_commit(tr);
		if ((fdb_future_block_until_ready(status)) != 0) {
			std::cerr << "fdb_future_block_until_ready: failed\n";
			return -1;
		}
		int error = fdb_future_get_error(status);
		if (error != 0) {
			std::cerr << "fdb_future_get_error: " << error << '\n';
			return -1;
		}
		fdb_future_destroy(status);
		fdb_transaction_destroy(tr);
		status = NULL;
		
		tr = NULL;
		if (fdb_database_create_transaction(db, &tr) != 0) {
			std::cerr << "fdb_database_create_transaction: failed\n";
			return -1;
		}
		fdb_transaction_set(tr, &key, sizeof(key), &key, sizeof(key));
		status = fdb_transaction_commit(tr);
		if ((fdb_future_block_until_ready(status)) != 0) {
			std::cerr << "fdb_future_block_until_ready: failed\n";
			return -1;
		}
		error = fdb_future_get_error(status);
		if (error != 0) {
			std::cerr << "fdb_future_get_error: " << error << '\n';
			return -1;
		}
		fdb_future_destroy(status);
		fdb_transaction_destroy(tr);
		status = NULL;
		tr = NULL;
	}
	fdb_is_initialized = 1;
	return 0;
}

inline int get_branch_indexes(std::vector<uint16_t> &branch_indexes, uint16_t leaf_id)
{
	if (leaf_id == 0 || leaf_id > 0x00008000)
		return -1;
	--leaf_id;
	uint16_t tree_idx = 1;
	for (uint8_t i = 0; i < 16; ++i) {
		branch_indexes.push_back(tree_idx);
		if (leaf_id & 0x00004000)
			tree_idx = tree_idx * 2 + 1;
		else
			tree_idx = tree_idx * 2;
		leaf_id <<= 1;
	}
	return 0;
}

inline int get_branch_from_fdb(std::vector<Block> &branch, std::vector<uint16_t> &branch_indexes)
{
	for (uint16_t current_bucket = 0; current_bucket < branch.size() / BLOCKS_PER_BUCKET; ++current_bucket) {
		// create transaction
		if (fdb_database_create_transaction(db, &tr) != 0) {
			std::cerr << "server: fdb_database_create_transaction: failed\n";
			return -1;
		}

		// request and wait for bucket
		uint8_t temp[3];
		temp[0] = 0;
		temp[1] = (branch_indexes[current_bucket] & 0xff00) >> 8;
		temp[2] = branch_indexes[current_bucket] & 0x00ff;
		status = fdb_transaction_get(tr, temp, sizeof(temp), 0);
		if (fdb_future_block_until_ready(status) != 0) {
			std::cerr << "server: fdb_future_block_until_ready: failed\n";
			return -1;
		}
		if (fdb_future_get_error(status)) {
			std::cerr << "server: fdb_future_is_error: its an error...\n";
			return -1;
		}

		// extract buckets
		fdb_bool_t out_present = 0;
		const uint8_t *out_value = NULL;
		int out_value_length = 0;
		if (fdb_future_get_value(status, &out_present, &out_value, &out_value_length) == 0) {
			if (out_present) {
				for (int current_block = 0; current_block < BLOCKS_PER_BUCKET; ++current_block) {
					branch[current_bucket * BLOCKS_PER_BUCKET + current_block].set_encrypted_data(out_value + current_block * BLOCK_SIZE, BLOCK_SIZE);
				}
			} else {
			}
		} else {
			std::cerr << "server: fdb_future_get_value: failed\n";
			return -1;
		}

		// clean up
		fdb_future_destroy(status);
		fdb_transaction_destroy(tr);
		status = NULL;
		tr = NULL;
	}
	return 0;
}

inline int send_branch_to_client(std::vector<Block> &branch)
{
	// send all data in branch
	for (uint32_t i = 0; i < branch.size() - 1; ++i) {
		if (send(client_socket, branch[i].get_encrypted_data(), BLOCK_SIZE, MSG_MORE) != BLOCK_SIZE) {
			perror("server: send: failed");
			return -1;
		}
	}
	if (send(client_socket, branch.back().get_encrypted_data(), BLOCK_SIZE, 0) != BLOCK_SIZE) {
		perror("server: send: failed");
		return -1;
	}
#ifdef DEBUG
	std::cout << "server: sent " << branch.size() << " encrypted blocks\n";
#endif

	return 0;
}

inline int receive_updated_blocks(std::vector<Block> &branch)
{
	// receive + update all data from branch
	for (uint32_t i = 0; i < branch.size(); ++i) {
		if (recv(client_socket, branch[i].get_encrypted_data(), BLOCK_SIZE, MSG_WAITALL) != BLOCK_SIZE) {
			perror("server: recv: failed");
			return -1;
		}
	}
#ifdef DEBUG
	std::cout << "server: recv updated encrypted data\n";
#endif

	return 0;
}

inline int send_branch_to_fdb(std::vector<Block> &branch, std::vector<uint16_t> &branch_indexes)
{
	// create transaction
	if (fdb_database_create_transaction(db, &tr) != 0) {
		std::cerr << "server: fdb_database_create_transaction: failed\n";
		return -1;
	}

	for (uint16_t current_bucket = 0; current_bucket < branch.size() / BLOCKS_PER_BUCKET; ++current_bucket) {
		// construct bucket
		uint8_t bucket[BLOCK_SIZE * BLOCKS_PER_BUCKET];
		for (uint8_t current_block = 0; current_block < BLOCKS_PER_BUCKET; ++current_block) {
			memcpy(bucket + current_block * BLOCK_SIZE, branch[current_bucket * BLOCKS_PER_BUCKET + current_block].get_encrypted_data(), BLOCK_SIZE);
		}

		// send bucket to fdb
		uint8_t temp[3];
		temp[0] = 0;
		temp[1] = (branch_indexes[current_bucket] & 0xff00) >> 8;
		temp[2] = branch_indexes[current_bucket] & 0x00ff;
		fdb_transaction_set(tr, temp, sizeof(temp), bucket, sizeof(bucket));
	}

	status = fdb_transaction_commit(tr);
	if ((fdb_future_block_until_ready(status)) != 0) {
		std::cerr << "server: fdb_future_block_until_ready: failed\n";
		return -1;
	}
	if (fdb_future_get_error(status)) {
		std::cerr << "server: fdb_future_is_error: its an error...\n";
		return -1;
	}
	fdb_future_destroy(status);
	fdb_transaction_destroy(tr);
	status = NULL;
	tr = NULL;

	return 0;
}

inline void close_connection()
{
	if (status != NULL)
		fdb_future_destroy(status);
	if (tr != NULL)
		fdb_transaction_destroy(tr);
	close(client_socket);
}
