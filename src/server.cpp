#include "tree.h"
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

inline int send_branch();
inline int recv_branch();
inline int update_fdb();

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
		std::cerr << "server: socket: failed\n";
		exit(EXIT_FAILURE);
	}

	int opt = 1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		std::cerr << "server: setsockopt: failed\n";
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);
	
	if (bind(server_socket, (struct sockaddr *) &address, sizeof(address)) < 0) {
		std::cerr << "server: bind: failed\n";
		exit(EXIT_FAILURE);
	}

	if (listen(server_socket, 3) < 0) {
		std::cerr << "server: listen: failed\n";
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

	// create tree structure in an array
	Tree tree(UINT16_MAX);

	// example nodes
	std::array<uint8_t, BYTES_PER_BLOCK> temp; // 2^16-1 nodes with 2^15 leafs with levels 0-15
	temp.fill(1);
	// int Tree::set_block(Block src_block, uint16_t bucket_index, uint16_t block_index)
	tree.set_block(Block(4, temp), 0, 1); // tree[0][1] => leaf_id = 4, data = temp
	tree.set_block(Block(1, temp), 1, 0); // tree[1][0] => leaf_id = 1, data = temp

	while (1) {
		LISTEN:
		if ( (client_socket = accept(server_socket, (struct sockaddr *) &address, &addr_len)) < 0 ) {
			std::cerr << "server: accept: failed\n";
			exit(EXIT_FAILURE);
		}
		struct in_addr ip_addr = address.sin_addr;
		std::string str(INET_ADDRSTRLEN, '\0');
		inet_ntop( AF_INET, &ip_addr, str.data(), INET_ADDRSTRLEN );
		std::cout << "server: connection to " << str << " established\n";

		while (1) {
			struct FDB_transaction *tr = NULL;
			struct FDB_future *status = NULL;

			// receive leaf_id
			uint16_t leaf_id;
			switch (recv(client_socket, &leaf_id, sizeof(leaf_id), 0)) {
			case 0:
				if (status != NULL)
					fdb_future_destroy(status);
				if (tr != NULL)
					fdb_transaction_destroy(tr);
				std::cout << "server: client disconnected\n";
				close(client_socket);
				goto LISTEN;
				break;
			case sizeof(leaf_id):
				break;
			default:
				std::cerr << "server: recv: failed\n";
				exit(EXIT_FAILURE);
			}
			std::cout << "server: leaf_id = " << leaf_id << '\n';

			// find branch given leaf_id
			std::vector<Block> branch;
			if (tree.get_path(branch, leaf_id) != 0) {
				std::cerr << "server: get_path: failed\n";
				exit(EXIT_FAILURE);
			}

			// send number of blocks to send
			uint16_t num_blocks = branch.size();
			if (send(client_socket, &num_blocks, sizeof(num_blocks), 0) != sizeof(num_blocks)) {
				std::cerr << "server: send: failed\n";
				exit(EXIT_FAILURE);
			}
			std::cout << "server: sent num_blocks = " << num_blocks << '\n';

			// send all leaf_ids in branch
			for (Block &block : branch) {
				uint16_t leaf_id = block.get_leaf_id();
				if (send(client_socket, &leaf_id, sizeof(leaf_id), 0) != sizeof(leaf_id)) {
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
		}
	}

	close(client_socket);
	shutdown(server_socket, SHUT_RDWR);

	return 0;
}
