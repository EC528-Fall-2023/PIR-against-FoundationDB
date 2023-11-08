#include "block.h"
#include "single_client.h"

#define PORT 8081

inline int setup_socket(struct sockaddr_in &master_addr);
inline int receive_request(uint16_t &request_id, uint8_t &operation);
int receive_key(std::string &key);

int client_socket = -1;
int master_socket = -1;

int main()
{
	SingleClient client;

	struct sockaddr_in master_addr, client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	if (setup_socket(master_addr) != 0) {
		std::cerr << "master: setup_socket failed\n";
		exit(EXIT_FAILURE);
	}

	while (1) {
		uint16_t request_id;
		uint8_t operation;

		if ( (client_socket = accept(master_socket, (struct sockaddr *) &client_addr, &client_addr_len)) < 0 ) {
			std::cerr << "master: accept failed\n";
			continue;
		}
#ifdef DEBUG
		std::cout << "master: connection to " << client_addr.sin_addr.s_addr << " established\n";
#endif

		if (receive_request(request_id, operation) != 0) {
			std::cerr << "master: receive_request failed\n";
			close(client_socket);
			continue;
		}

		std::string key1;
		std::string key2;
		std::array<uint8_t, BYTES_PER_BLOCK> data_buffer;

		switch (operation) {
		case READ:
			if (receive_key(key1) != 0 || client.get(key1, data_buffer) != 0 || send(client_socket, data_buffer.data(), BYTES_PER_BLOCK, 0) != BYTES_PER_BLOCK) {
				std::cerr << "master: READ failed\n";
				close(client_socket);
				continue;
			}
			break;
		case WRITE:
			if (receive_key(key1) != 0 || client.put(key1, data_buffer) != 0) {
				std::cerr << "master: WRITE failed\n";
				close(client_socket);
				continue;
			}
			break;
		case CLEAR:
			if (receive_key(key1) != 0 || client.clear(key1) != 0) {
				std::cerr << "master: CLEAR failed\n";
				close(client_socket);
				continue;
			}
			break;
		case READ_RANGE: {
			std::vector<std::array<uint8_t, BYTES_PER_BLOCK>> blocks;
			if (receive_key(key1) != 0 || receive_key(key2) != 0 || client.read_range(key1, key2, blocks)) {
				std::cerr << "master: READ_RANGE failed\n";
				close(client_socket);
				continue;
			}
			uint32_t num_blocks = blocks.size();
			if (send(client_socket, &num_blocks, sizeof(num_blocks), 0) != sizeof(num_blocks)) {
				std::cerr << "master: READ_RANGE failed\n";
				close(client_socket);
				continue;
			}
			for (uint32_t i = 0; i < num_blocks; ++i) {
				if (send(client_socket, blocks[i].data(), BYTES_PER_BLOCK, 0) != BYTES_PER_BLOCK) {
					std::cerr << "master: READ_RANGE failed\n";
					close(client_socket);
					continue;
				}
			}
			break;
		} case CLEAR_RANGE:
			if (receive_key(key1) != 0 || receive_key(key2) != 0 || client.clear_range(key1, key2) != 0) {
				std::cerr << "master: CLEAR_RANGE failed\n";
				close(client_socket);
				continue;
			}
			break;
		default:
			std::cerr << "master: invalid operation\n";
			close(client_socket);
			continue;
		}

		// confirm request
		if (send(client_socket, &request_id, sizeof(request_id), 0) != sizeof(request_id)) {
			std::cerr << "master: send failed\n";
			close(client_socket);
			continue;
		}

		close(client_socket);
	} // while (1)

	return 0;
}

inline int setup_socket(struct sockaddr_in &master_addr)
{
	if ( (master_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		std::cerr << "master: socket failed\n";
		return -1;
	}

	int opt = 1;
	if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		std::cerr << "master: setsockopt failed\n";
		close(master_socket);
		return -1;
	}

	memset(&master_addr, 0, sizeof(master_addr));
	master_addr.sin_family = AF_INET;
	master_addr.sin_addr.s_addr = INADDR_ANY;
	master_addr.sin_port = htons(PORT);
	
	if (bind(master_socket, (struct sockaddr *) &master_addr, sizeof(master_addr)) < 0) {
		std::cerr << "master: bind failed\n";
		close(master_socket);
		return -1;
	}

	if (listen(master_socket, 3) < 0) {
		std::cerr << "master: listen failed\n";
		close(master_socket);
		return -1;
	}

	return 0;
}

inline int receive_request(uint16_t &request_id, uint8_t &operation)
{
	if (recv(client_socket, &request_id, sizeof(request_id), 0) != sizeof(request_id)) {
		std::cerr << "master: recv failed\n";
		return -1;
	}

	if (recv(client_socket, &operation, sizeof(operation), 0) != sizeof(operation)) {
		std::cerr << "master: recv failed\n";
		return -1;
	}

	return 0;
}

int receive_key(std::string &key)
{
	uint32_t key_size;
	if (recv(client_socket, &key_size, sizeof(key_size), 0) != sizeof(key_size)) {
		std::cerr << "master: recv failed\n";
		return -1;
	}
	key.resize(key_size);

	if (recv(client_socket, key.data(), key_size, 0) != key_size) {
		std::cerr << "master: recv failed\n";
		return -1;
	}
#ifdef DEBUG
	std::cout << "master: recv " << key << " from client";
#endif

	return 0;
}
