#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080

#define FDB_API_VERSION 710
#include <foundationdb/fdb_c.h>

inline int setup_socket(int &server_socket, struct sockaddr_in &address);
inline int get_branch_indexes(std::vector<uint16_t> &branch_indexes, uint16_t leaf_id);

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
	int server_socket, client_socket;
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	if (setup_socket(server_socket, server_addr) != 0) {
		std::cerr << "setup_socket: failed\n";
		exit(EXIT_FAILURE);
	}

	pthread_t network_thread;
	struct FDB_database *db = NULL;
	struct FDB_transaction *tr = NULL;
	struct FDB_future *status = NULL;

	if (setup_fdb(network_thread, &db) != 0) {
		std::cerr << "setup_fdb: failed\n";
		exit(EXIT_FAILURE);
	}


/*
	// create tree structure in an array
	Tree tree(UINT16_MAX);

	// example nodes
	std::array<uint8_t, BYTES_PER_BLOCK> temp; // 2^16-1 nodes with 2^15 leafs with levels 0-15
	temp.fill(1);
	// int Tree::set_block(Block src_block, uint16_t bucket_index, uint16_t block_index)
	tree.set_block(Block(4, temp), 0, 1); // tree[0][1] => leaf_id = 4, data = temp
	tree.set_block(Block(1, temp), 1, 0); // tree[1][0] => leaf_id = 1, data = temp
*/

	while (1) {
		if ( (client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_addr_len)) < 0 ) {
			std::cerr << "server: accept: failed\n";
			exit(EXIT_FAILURE);
		}
		std::string str(INET_ADDRSTRLEN, '\0');
		inet_ntop(AF_INET, &client_addr.sin_addr, str.data(), INET_ADDRSTRLEN);
		std::cout << "server: connection to " << str << " established\n";

		// receive leaf_id
		uint16_t leaf_id;
		switch (recv(client_socket, &leaf_id, sizeof(leaf_id), 0)) {
/*
		case 0:
			if (status != NULL)
				fdb_future_destroy(status);
			if (tr != NULL)
				fdb_transaction_destroy(tr);
			std::cout << "server: client disconnected\n";
			close(client_socket);
			break;
*/
		case sizeof(leaf_id):
			break;
		default:
			std::cerr << "server: recv: failed\n";
			exit(EXIT_FAILURE);
		}
		std::cout << "server: leaf_id = " << leaf_id << '\n';

		// find indexes that correspond to nodes on the branch
		std::vector<uint16_t> branch_indexes;
		if (get_branch_indexes(branch_indexes, leaf_id) != 0) {
			std::cerr << "server: get_branch_indexes: failed";
			exit(EXIT_FAILURE);
		}

		// send number of blocks to send
		uint16_t num_blocks = branch_indexes.size();
		if (send(client_socket, &num_blocks, sizeof(num_blocks), 0) != sizeof(num_blocks)) {
			std::cerr << "server: send: failed\n";
			exit(EXIT_FAILURE);
		}
		std::cout << "server: sent num_blocks = " << num_blocks << '\n';

		// retrieve vector<Block> from fdb
		std::vector<Block> branch(num_blocks);
		for (uint16_t i = 0; i < num_blocks; ++i) {
			// create transaction
			if (fdb_database_create_transaction(db, &tr) != 0) {
				std::cerr << "fdb_database_create_transaction: failed\n";
				exit(EXIT_FAILURE);
			}

			// request and wait for bucket
			struct FDB_future *status = fdb_transaction_get(tr, &branch_indexes[i], sizeof(uint16_t), 0);
			if (fdb_future_block_until_ready(status) != 0) {
				std::cerr << "fdb_future_block_until_ready: failed\n";
				exit(EXIT_FAILURE);
			}
			if (fdb_future_get_error(status)) {
				std::cerr << "fdb_future_is_error: its an error...\n";
				exit(EXIT_FAILURE);
			}

			// extract buckets
			fdb_bool_t out_present = 0;
			const uint8_t *out_value = NULL;
			int out_value_length = 0;
			if (fdb_future_get_value(status, &out_present, &out_value, &out_value_length) == 0 && out_present) {
				for (int j = 0; j < BLOCKS_PER_BUCKET; ++j) {
					uint16_t temp_leaf_id = 0;
					for (int k = sizeof(uint8_t); k <= sizeof(temp_leaf_id); ++k) {
						temp_leaf_id |= out_value[k-1] << ((sizeof(temp_leaf_id) - k) * 8);
					}
					std::array<uint8_t, BYTES_PER_BLOCK> data_buffer;
					memcpy(data_buffer.data(), out_value + sizeof(temp_leaf_id) + j * (BYTES_PER_BLOCK + sizeof(temp_leaf_id)), BYTES_PER_BLOCK);
					branch[i * BLOCKS_PER_BUCKET + j] = Block(temp_leaf_id, data_buffer);
				}
			} else {
				std::cerr << "fdb_future_get_value: failed\n";
				exit(EXIT_FAILURE);
			}
		}

		// send all leaf_ids in branch
		for (Block &block : branch) {
			uint16_t temp_leaf_id = block.get_leaf_id();
			if (send(client_socket, &temp_leaf_id, sizeof(temp_leaf_id), 0) != sizeof(temp_leaf_id)) {
				std::cerr << "server: send: failed\n";
				exit(EXIT_FAILURE);
			}
		}
		std::cout << "server: sent leaf_ids\n";

		// send all data in branch
		for (Block &block : branch) {
			if (send(client_socket, block.get_data().data(), BYTES_PER_BLOCK, 0) != BYTES_PER_BLOCK) {
				std::cerr << "server: send: failed\n";
				exit(EXIT_FAILURE);
			}
		}
		std::cout << "server: sent " << branch.size() << " blocks\n";

		// receive + update all leaf_id from branch
		for (Block &block : branch) {
			uint16_t leaf_id;
			if (recv(client_socket, &leaf_id, sizeof(leaf_id), 0) != sizeof(leaf_id)) {
				std::cerr << "server: recv: failed\n";
				exit(EXIT_FAILURE);
			}
			block.set_leaf_id(leaf_id);
		}
		std::cout << "server: leaf_ids =";
		for (unsigned long int i = 0; i < branch.size(); ++i) {
			if (i % 3 == 0)
				std::cout << " |";
			std::cout << ' ' << branch[i].get_leaf_id();
		}
		std::cout << "\nserver: recv updated leaf_ids\n";

		// receive + update all data from branch
		for (Block &block : branch) {
			std::array<uint8_t, BYTES_PER_BLOCK> data_buffer;
			if (recv(client_socket, data_buffer.data(), BYTES_PER_BLOCK, 0) != BYTES_PER_BLOCK) {
				std::cerr << "server: recv: failed\n";
				exit(EXIT_FAILURE);
			}
			block.set_data(data_buffer);
			
		}
		std::cout << "server: recv updated data\n";

		
		if (fdb_database_create_transaction(db, &tr) != 0) {
			std::cerr << "fdb_database_create_transaction: failed\n";
			exit(EXIT_FAILURE);
		}


		if (status != NULL)
			fdb_future_destroy(status);
		if (tr != NULL)
			fdb_transaction_destroy(tr);
		close(client_socket);
	}

	shutdown(server_socket, SHUT_RDWR);

	fdb_stop_network();
	pthread_join(network_thread, NULL);

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

inline int setup_fdb(pthread_t &network_thread, struct fdb_database **db)
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

	if (fdb_create_database(NULL, db) != 0) {
		std::cerr << "fdb_create_database: failed\n";
		return -1;
	}

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


