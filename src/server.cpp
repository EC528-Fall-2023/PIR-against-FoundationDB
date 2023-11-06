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

inline int setup_socket(int &server_socket, struct sockaddr_in &address);
inline int setup_fdb(pthread_t &network_thread);
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
bool fdb_is_initialized = false;

int main()
{
	int server_socket;
	struct sockaddr_in server_addr, client_addr;

	struct timeval start_time, end_time;
	gettimeofday(&start_time, NULL);

	socklen_t client_addr_len = sizeof(client_addr);

	if (setup_socket(server_socket, server_addr) != 0) {
		std::cerr << "setup_socket: failed\n";
		exit(EXIT_FAILURE);
	}

	pthread_t network_thread;
	if (setup_fdb(network_thread) != 0) {
		std::cerr << "setup_fdb: failed\n";
		exit(EXIT_FAILURE);
	}

	gettimeofday(&end_time, NULL);
	time_t elapsed_seconds = end_time.tv_sec - start_time.tv_sec;
	if (!fdb_is_initialized)
		std::cout << "server: " << elapsed_seconds << " seconds on first ever initialization\n";
	else
		std::cout << "server: " << elapsed_seconds << " seconds on first ever initialization\n";

	while (1) {
		LISTEN:
		if ( (client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_addr_len)) < 0 ) {
			std::cerr << "server: accept: failed\n";
			continue;
		}
		char client_ip[INET_ADDRSTRLEN] = {0};
		inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
		std::cout << "server: connection to " << client_ip << " established\n";

		while (1) {
			// receive request_id
			uint16_t request_id;
			switch (recv(client_socket, &request_id, sizeof(request_id), 0)) {
			case 0:
				std::cout << "server: client disconnected\n";
				goto LISTEN;
			case sizeof(request_id):
				break;
			default:
				std::cerr << "server: recv: failed\n";
				close_connection();
				goto LISTEN;
			}
			std::cout << "server: request_id = " << request_id << '\n';

			// receive leaf_id
			uint16_t leaf_id;
			if (recv(client_socket, &leaf_id, sizeof(leaf_id), 0) != sizeof(leaf_id)) {
				std::cerr << "server: recv: failed\n";
				close_connection();
				continue;
			}
			std::cout << "server: leaf_id = " << leaf_id << '\n';

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
				std::cerr << "server: send: failed\n";
				close_connection();
				continue;
			}
			std::cout << "server: sent num_blocks = " << num_blocks << '\n';

			// retrieve vector<Block> from fdb
			std::vector<Block> branch(num_blocks);
			if (get_branch_from_fdb(branch, branch_indexes) != 0) {
				std::cerr << "server: get_branch_from fdb\n";
				close_connection();
				continue;
			}
			std::cout << "server: retrieved branch from fdb\n";

			if (send_branch_to_client(branch) != 0) {
				std::cerr << "server: send_branch_to_clien: failed\n";
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
				std::cerr << "server: send_branch_to_fdb: failed\n";
				close_connection();
				continue;
			}

			// send acknowledgement of successful operation
			if (send(client_socket, &request_id, sizeof(request_id), 0) != sizeof(request_id)) {
				std::cerr << "server: send: failed acknowledgement\n";
				close_connection();
				continue;
			}

			// print results
			std::cout << "server: showing " << num_blocks << " blocks, their leaf_ids and first 10 bytes of the data\n";
			for (unsigned long i = 0; i < branch.size(); ++i) {
				if (i % 3 == 0)
					std::cout << "---------------------------\n";
				std::cout << branch[i].get_block_id() << ',' << '\t';
				for (int j = 0; j < 10; ++j) {
					char c = branch[i].get_data()[j];
					if (c < 32 || c > 126)
						c = '.';
					std::cout << c << ' ';
				}
				std::cout << '\n';
			}
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

inline int setup_socket(int &server_socket, struct sockaddr_in &address)
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

	return 0;
}

inline int setup_fdb(pthread_t &network_thread)
{
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

	// check if tree was initiated already by checking if key: \x01 = value: \x01
	if (fdb_database_create_transaction(db, &tr) != 0) {
		std::cerr << "fdb_database_create_transaction: failed\n";
		return -1;
	}

	uint8_t key = 1;
	status = fdb_transaction_get(tr, &key, sizeof(key), 0);

	if ((fdb_future_block_until_ready(status)) != 0) {
		perror("fdb_future_block_until_ready: failed");
		return -1;
	}
	if (fdb_future_get_error(status)) {
		perror("fdb_future_is_error: its an error...");
		return -1;
	}

	bool initialized = 0;
	fdb_bool_t out_present = 0;
	const uint8_t *out_value = NULL;
	int out_value_length = 0;
	if (fdb_future_get_value(status, &out_present, &out_value, &out_value_length) == 0) {
		if (out_present == 0)
			initialized = 0;
		else if (out_value[0] == 1)
			initialized = 1;
		else
			initialized = 0;
	} else {
		std::cerr << "fdb_future_get_value: failed\n";
		return -1;
	}
	fdb_future_destroy(status);
	fdb_transaction_destroy(tr);
	status = NULL;
	tr = NULL;

	if (!initialized) {
		for (uint16_t tree_index = 1; tree_index != 0; ++tree_index) {
			tr = NULL;
			if (fdb_database_create_transaction(db, &tr) != 0) {
				std::cerr << "fdb_database_create_transaction: failed\n";
				return -1;
			}

			uint8_t temp[3] = {0};
			temp[1] = (tree_index & 0xff00) >> 8;
			temp[2] = tree_index & 0x00ff;
			uint16_t leaf_id = 0;
			std::array<uint8_t, (sizeof(leaf_id) + BYTES_PER_BLOCK) * BLOCKS_PER_BUCKET> bucket;
			for (uint8_t &byte : bucket) {
				byte = random_byte(generator);
			}
			for (int block = 0; block < BLOCKS_PER_BUCKET; ++block) {
				memcpy(bucket.data() + (sizeof(leaf_id) + BYTES_PER_BLOCK) * block, &leaf_id, sizeof(leaf_id));
			}
			fdb_transaction_set(tr, (const uint8_t *) temp, sizeof(temp), (const uint8_t *) bucket.data(), bucket.size());
			status = fdb_transaction_commit(tr);
			if ((fdb_future_block_until_ready(status)) != 0) {
				std::cerr << "fdb_future_block_until_ready: failed\n";
				return -1;
			}
			int error = fdb_future_get_error(status);
			if (error != 0) {
				std::cout << "fdb_future_get_error: " << error << '\n';
				return -1;
			}
			fdb_future_destroy(status);
			fdb_transaction_destroy(tr);
			status = NULL;
			tr = NULL;

			if (tree_index % 5000 == 0)
				std::cout << "server: initialized " << tree_index << " buckets\n";
		}
		
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
		int error = fdb_future_get_error(status);
		if (error != 0) {
			std::cout << "fdb_future_get_error: " << error << '\n';
			return -1;
		}
		fdb_future_destroy(status);
		fdb_transaction_destroy(tr);
		status = NULL;
		tr = NULL;
	}
	fdb_is_initialized = true;
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
					uint16_t temp_leaf_id = *((uint16_t *) (out_value + current_block * (sizeof(temp_leaf_id) + BYTES_PER_BLOCK)));
					//uint16_t temp_leaf_id;
					//memcpy(&temp_leaf_id, out_value + current_block * (sizeof(temp_leaf_id) + BYTES_PER_BLOCK), sizeof(temp_leaf_id));
					std::array<uint8_t, BYTES_PER_BLOCK> data_buffer;
					memcpy(data_buffer.data(), out_value + sizeof(temp_leaf_id) + current_block * (BYTES_PER_BLOCK + sizeof(temp_leaf_id)), BYTES_PER_BLOCK);
					branch[current_bucket * BLOCKS_PER_BUCKET + current_block] = Block(temp_leaf_id, data_buffer);
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
	// send all leaf_ids in branch
	std::vector<uint16_t> leaf_ids(branch.size());
	for (unsigned long i = 0; i < branch.size(); ++i) {
		leaf_ids[i] = branch[i].get_block_id();
	}
	if (send(client_socket, leaf_ids.data(), sizeof(uint16_t) * leaf_ids.size(), 0) != (long int) (leaf_ids.size() * sizeof(uint16_t))) {
		std::cerr << "server: send: failed\n";
		return -1;
	}
	std::cout << "server: sent leaf_ids\n";

	// send all data in branch
	for (Block &block : branch) {
		if (send(client_socket, block.get_data().data(), BYTES_PER_BLOCK, 0) != BYTES_PER_BLOCK) {
			std::cerr << "server: send: failed\n";
			return -1;
		}
	}
	std::cout << "server: sent " << branch.size() << " blocks\n";

	return 0;
}

inline int receive_updated_blocks(std::vector<Block> &branch)
{
	// receive all updated leaf_ids
	std::vector<uint16_t> leaf_ids(branch.size(), 0);
	if (recv(client_socket, leaf_ids.data(), sizeof(uint16_t) * leaf_ids.size(), 0) != (long int) (sizeof(uint16_t) * leaf_ids.size())) {
		std::cerr << "server: recv: failed\n";
		return -1;
	}

	// receive + update all data from branch
	for (unsigned long i = 0; i < branch.size(); ++i) {
		std::array<uint8_t, BYTES_PER_BLOCK> data_buffer;
		if (recv(client_socket, data_buffer.data(), BYTES_PER_BLOCK, 0) != BYTES_PER_BLOCK) {
			std::cerr << "server: recv: failed\n";
			return -1;
		}
		branch[i].set_block_id(leaf_ids[i]);
		branch[i].set_data(data_buffer);
	}
	std::cout << "server: recv updated leaf_ids\n";
	std::cout << "server: recv updated data\n";

	return 0;
}

inline int send_branch_to_fdb(std::vector<Block> &branch, std::vector<uint16_t> &branch_indexes)
{
	for (uint16_t current_bucket = 0; current_bucket < branch.size() / BLOCKS_PER_BUCKET; ++current_bucket) {
		// create transaction
		if (fdb_database_create_transaction(db, &tr) != 0) {
			std::cerr << "server: fdb_database_create_transaction: failed\n";
			return -1;
		}

		// construct bucket
		std::array<uint8_t, (sizeof(uint16_t) + BYTES_PER_BLOCK) * BLOCKS_PER_BUCKET> bucket;
		for (uint8_t current_block = 0; current_block < BLOCKS_PER_BUCKET; ++current_block) {
			uint16_t temp_leaf_id = branch[current_bucket * BLOCKS_PER_BUCKET + current_block].get_block_id();
			*((uint16_t *) (bucket.data() + current_block * (sizeof(temp_leaf_id) + BYTES_PER_BLOCK))) = temp_leaf_id;
			//memcpy(bucket.data() + current_block * (sizeof(temp_leaf_id) + BYTES_PER_BLOCK), &temp_leaf_id, sizeof(temp_leaf_id));
			memcpy(bucket.data() + sizeof(temp_leaf_id) + current_block * (sizeof(temp_leaf_id) + BYTES_PER_BLOCK), branch[current_bucket * BLOCKS_PER_BUCKET + current_block].get_data().data(), BYTES_PER_BLOCK);
		}

		// send bucket to fdb
		uint8_t temp[3];
		temp[0] = 0;
		temp[1] = (branch_indexes[current_bucket] & 0xff00) >> 8;
		temp[2] = branch_indexes[current_bucket] & 0x00ff;
		fdb_transaction_set(tr, temp, sizeof(temp), bucket.data(), (sizeof(uint16_t) + BYTES_PER_BLOCK) * BLOCKS_PER_BUCKET);
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
	}

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
