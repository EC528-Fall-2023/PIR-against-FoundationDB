#include "single_client.h"

static std::random_device generator;
static std::uniform_int_distribution<int> oram_random(1, 32768); // [1, 2^15]

SingleClient::SingleClient(const std::string &server_ip, const int port)
{
	if ( (socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		printf("socket: failed\n");
		exit(EXIT_FAILURE);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
		printf("inet_pton: failed\n");
		//close(socket_fd);
		exit(EXIT_FAILURE);
	}

	if (connect(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0 ) {
		printf("single_client: connect: failed\n");
		exit(EXIT_FAILURE);
	}
#ifdef DEBUG
	std::cout << "single_client: connection to " << server_ip << ':' << port << " established\n";
#endif

	if (access(".oram_state", F_OK) == 0) {
		int state_fd;
		if ( (state_fd = open(".oram_state", O_RDONLY | O_CREAT, 0666)) == -1) {
			std::cerr << "single_client: open state: failed " << errno << '\n';
			exit(EXIT_FAILURE);
		}

		std::string key;
		uint16_t block_id;
		uint16_t leaf_id;
		unsigned int size;

		read(state_fd, &size, sizeof(size));
		for (unsigned int i = 0; i < size; ++i) {
			key.clear();
			unsigned int string_size;
			read(state_fd, &string_size, sizeof(string_size));
			key.resize(string_size);
			read(state_fd, key.data(), string_size);
			read(state_fd, &block_id, sizeof(block_id));
			key_to_block_id[key] = block_id;
		}

		read(state_fd, &size, sizeof(size));
		for (unsigned int i = 0; i < size; ++i) {
			read(state_fd, &block_id, sizeof(block_id));
			read(state_fd, &leaf_id, sizeof(leaf_id));
			position_map[block_id] = leaf_id;
		}

		read(state_fd, &size, sizeof(size));
		for (unsigned int i = 0; i < size; ++i) {
			std::array<uint8_t, BYTES_PER_BLOCK> temp;
			read(state_fd, &block_id, sizeof(block_id));
			read(state_fd, temp.data(), BYTES_PER_BLOCK);
			stash[block_id] = temp;
		}

		close(state_fd);
	}
}

SingleClient::~SingleClient()
{
	close(socket_fd);
#ifdef DEBUG
	std::cout << "single_client: disconnected\n";
#endif
	store_state();
	// save position map and stash to disk
}

int SingleClient::put(const std::string &key_name, const std::array<uint8_t, BYTES_PER_BLOCK> &value)
{
	// if key does not exist yet, add new entry
	if (key_to_block_id.find(key_name) == key_to_block_id.end()) {
		unsigned long int i;
		for (i = 1; i <= position_map.size(); ++i) {
			if (position_map.find(i) == position_map.end())
				break;
		}
		key_to_block_id[key_name] = i;
		position_map[i] = oram_random(generator);
	}

	uint16_t requested_block_id = key_to_block_id[key_name];
	
	uint16_t request_id = oram_random(generator);
	if (send(socket_fd, &request_id, sizeof(request_id), 0) != sizeof(request_id)) {
		perror("single_client: send: failed");
		close(socket_fd);
		return -1;
	}
#ifdef DEBUG
	std::cout << "single_client: request_id = " << request_id << '\n';
#endif

	if (fetch_branch(position_map[requested_block_id]) == -1) {
		std::cerr << "single_client: fetch_branch: failed" << std::endl;
		close(socket_fd);
		return -1;
	}

	// perform swaps
	std::array<uint8_t, BYTES_PER_BLOCK> value_buffer = value;
	traverse_branch(requested_block_id, WRITE, &value_buffer);

	// send branch back
	if (send_branch() == -1) {
		std::cerr << "single_client: send_branch: failed\n";
		close(socket_fd);
		return -1;
	}

	// confirm the operation completed
	uint16_t confirmation_id = 0;
	if (recv(socket_fd, &confirmation_id, sizeof(confirmation_id), 0) != sizeof(confirmation_id) || confirmation_id != request_id) {
		perror("single_client: recv: failed");
		close(socket_fd);
		return -1;
	}
#ifdef DEBUG
	std::cout << "single_client: PUT SUCCESS\n";
#endif
	store_state();
	branch.clear();

	return 0;
}

int SingleClient::get(const std::string &key_name, std::array<uint8_t, BYTES_PER_BLOCK> &value)
{
	if (key_to_block_id.find(key_name) == key_to_block_id.end()) {
#ifdef DEBUG
		std::cout << "key is not in position map\n";
#endif
		return -1;
	}

	uint16_t requested_block_id = key_to_block_id[key_name];

	uint16_t request_id = oram_random(generator);
	if (send(socket_fd, &request_id, sizeof(request_id), 0) != sizeof(request_id)) {
		perror("single_client: send: failed");
		close(socket_fd);
		return -1;
	}
#ifdef DEBUG
	std::cout << "single_client: request_id = " << request_id << '\n';
#endif

	if (fetch_branch(position_map[requested_block_id]) == -1) {
		std::cerr << "single_client: fetch_branch: failed" << std::endl;
		close(socket_fd);
		return -1;
	}

	// perform swaps
	std::array<uint8_t, BYTES_PER_BLOCK> value_buffer = {0};
	traverse_branch(requested_block_id, READ, &value_buffer);

	// send branch back
	if (send_branch() == -1) {
		std::cerr << "single_client: send_branch: failed\n";
		close(socket_fd);
		return -1;
	}

	// confirm the operation completed
	uint16_t confirmation_id = 0;
	if (recv(socket_fd, &confirmation_id, sizeof(confirmation_id), 0) != sizeof(confirmation_id) || confirmation_id != request_id) {
		perror("single_client: recv: failed");
		close(socket_fd);
		return -1;
	}

#ifdef DEBUG
	std::cout << "single_client: GET SUCCESS\n";
#endif
	store_state();
	memcpy(value.data(), value_buffer.data(), BYTES_PER_BLOCK);
	branch.clear();

	return 0;
}

int SingleClient::clear(const std::string &key_name)
{
	if (key_to_block_id.find(key_name) == key_to_block_id.end())
		return 0;

	uint16_t requested_block_id = key_to_block_id[key_name];
	
	uint16_t request_id = oram_random(generator);
	if (send(socket_fd, &request_id, sizeof(request_id), 0) != sizeof(request_id)) {
		perror("single_client: send: failed");
		close(socket_fd);
		return -1;
	}
#ifdef DEBUG
	std::cout << "single_client: request_id = " << request_id << '\n';
#endif

	if (fetch_branch(position_map[requested_block_id]) == -1) {
		std::cerr << "single_client: fetch_branch: failed" << std::endl;
		close(socket_fd);
		return -1;
	}

	// perform swaps
	traverse_branch(requested_block_id, CLEAR, nullptr);

	// send branch back
	if (send_branch() == -1) {
		std::cerr << "single_client: send_branch: failed\n";
		close(socket_fd);
		return -1;
	}

	// confirm the operation completed
	uint16_t confirmation_id = 0;
	if (recv(socket_fd, &confirmation_id, sizeof(confirmation_id), 0) != sizeof(confirmation_id) || confirmation_id != request_id) {
		perror("single_client: recv: failed");
		close(socket_fd);
		return -1;
	}
	position_map.erase(requested_block_id);
	key_to_block_id.erase(key_name);
#ifdef DEBUG
	std::cout << "single_client: CLEAR SUCCESS\n";
#endif
	store_state();
	branch.clear();

	return 0;
}

int SingleClient::read_range(const std::string &begin_key_name, const std::string &end_key_name, std::vector<std::array<uint8_t, BYTES_PER_BLOCK>> &data)
{
	for (auto map_iterator = key_to_block_id.lower_bound(begin_key_name); map_iterator != key_to_block_id.upper_bound(end_key_name); ++map_iterator) {
		std::array<uint8_t, BYTES_PER_BLOCK> data_buffer;
		if (get(map_iterator->first, data_buffer) != 0) {
			std::cerr << "single_client: read_range: failed. " << map_iterator->first << '\n';
			return -1;
		}
		data.push_back(data_buffer);
	}
	return 0;
}

int SingleClient::clear_range(const std::string &begin_key_name, const std::string &end_key_name)
{
	for (auto map_iterator = key_to_block_id.lower_bound(begin_key_name); map_iterator != key_to_block_id.upper_bound(end_key_name); ++map_iterator) {
		if (clear(map_iterator->first) != 0) {
			std::cerr << "single_client: clear_range: failed. " << map_iterator->first << '\n';
			return -1;
		}
	}
	return 0;
}

int SingleClient::fetch_branch(uint16_t leaf_id)
{
	if (send(socket_fd, &leaf_id, sizeof(leaf_id), 0) != sizeof(leaf_id)) {
		perror("single_client: send: failed");
		return -1;
	}
#ifdef DEBUG
	std::cout << "single_client: sent leaf_id = " << leaf_id << '\n';
#endif

	uint16_t num_blocks;
	if (recv(socket_fd, &num_blocks, sizeof(num_blocks), 0) != sizeof(num_blocks)) {
		perror("single_client: recv: failed");
		return -1;
	}
	branch.resize(num_blocks);
#ifdef DEBUG
	std::cout << "single_client: num_blocks = " << num_blocks << '\n';
#endif

	for (uint16_t i = 0; i < num_blocks; ++i) {
		if (recv(socket_fd, branch[i].get_encrypted_data(), BLOCK_ID_SIZE + BYTES_PER_BLOCK, MSG_WAITALL) != BLOCK_ID_SIZE + BYTES_PER_BLOCK) {
			perror("single_client: recv: failed");
			return -1;
		}
		branch[i].decrypt();
	}

#ifdef DEBUG
	std::cout << "single_client: leaf_ids =";
	for (int i = 0; i < num_blocks; ++i) {
		if (i % BLOCKS_PER_BUCKET == 0)
			std::cout << " |";
		std::cout << ' ' << branch[i].get_leaf_id();
	}
	std::cout << '\n';
	std::cout << "single_client: recv " << num_blocks << " blocks\n";
#endif

	return 0;
}

void SingleClient::traverse_branch(uint16_t requested_block_id, enum Operation op, std::array<uint8_t, BYTES_PER_BLOCK> *value_buffer)
{
	for (Block &current_block : branch) {
		if (current_block.get_block_id() == 0)
			continue;

		if (current_block.get_block_id() == requested_block_id) {
			if (op == READ)
				memcpy(value_buffer->data(), current_block.get_decrypted_data(), BYTES_PER_BLOCK);
			else if (op == WRITE)
				memcpy(current_block.get_decrypted_data(), value_buffer->data(), BYTES_PER_BLOCK);
			else if (op == CLEAR)
				current_block.set_block_id(0);

			uint16_t rand_leaf_id = oram_random(generator);
			uint16_t idx = find_intersection_bucket(position_map[requested_block_id], rand_leaf_id);
			position_map[requested_block_id] = rand_leaf_id;
#ifdef DEBUG
			std::cout << "single_client: found leaf_id " << requested_block_id << ", randomized to leaf_id " << rand_leaf_id << ", bucket idx " << idx << '\n';
#endif
			for (long j = (idx + 1) * BLOCKS_PER_BUCKET - 1; j >= 0; --j) {
				if (branch[j].get_block_id() == 0) {
					current_block.swap(branch[j]);
					return;
				}
			}
			// store in stash
			// free current block

			memcpy(stash[current_block.get_block_id()].data(), current_block.get_decrypted_data(), BYTES_PER_BLOCK);
#ifdef DEBUG
			std::cout << "wrote block " << current_block.get_block_id() << " to stash\n";
#endif
			current_block.set_block_id(0);
			current_block.set_decrypted_random_data();
			return;
		}
		uint16_t idx = find_intersection_bucket(position_map[requested_block_id], position_map[current_block.get_block_id()]);
		long j;
		for (j = idx * BLOCKS_PER_BUCKET; j < (idx + 1) * BLOCKS_PER_BUCKET; ++j) {
			if (branch[j].get_block_id() == 0) {
				current_block.swap(branch[j]);
				break;
			}
		}
		if (j == (idx + 1) * BLOCKS_PER_BUCKET) {
			// store in stash
			// free current block

			memcpy(stash[current_block.get_block_id()].data(), current_block.get_decrypted_data(), BYTES_PER_BLOCK);
#ifdef DEBUG
			std::cout << "wrote block " << current_block.get_block_id() << " to stash\n";
#endif
			current_block.set_block_id(0);
			current_block.set_decrypted_random_data();
		}
	}

	// look for requested block in the stash
	for (auto current_block = stash.begin(); current_block != stash.end(); ) {
		RESTART:
		if (current_block->first == requested_block_id) {
			if (op == READ)
				memcpy(value_buffer->data(), current_block->second.data(), BYTES_PER_BLOCK);
			else if (op == WRITE)
				memcpy(current_block->second.data(), value_buffer->data(), BYTES_PER_BLOCK);
			else if (op == CLEAR) {
				stash.erase(current_block);
				return;
			}
			uint16_t rand_leaf_id = oram_random(generator);
			uint16_t idx = find_intersection_bucket(position_map[requested_block_id], rand_leaf_id);
			position_map[requested_block_id] = rand_leaf_id;
#ifdef DEBUG
			std::cout << "single_client: found leaf_id in stash" << requested_block_id << ", randomized to leaf_id " << rand_leaf_id << ", bucket idx " << idx << '\n';
#endif
			for (long j = (idx + 1) * BLOCKS_PER_BUCKET - 1; j >= 0; --j) {
				if (branch[j].get_block_id() == 0) {
#ifdef DEBUG
					std::cout << "single_client: moved block " << current_block->first << " from stash to branch\n";
#endif
					branch[j] = Block(current_block->first, current_block->second.data(), BYTES_PER_BLOCK);
					stash.erase(current_block);
					break;
				}
			}
			return;
		}
		uint16_t idx = find_intersection_bucket(position_map[requested_block_id], position_map[current_block->first]);
		for (long j = idx * BLOCKS_PER_BUCKET; j < (idx + 1) * BLOCKS_PER_BUCKET; ++j) {
			if (branch[j].get_block_id() == 0) {
#ifdef DEBUG
				std::cout << "single_client: moved block " << current_block->first << " from stash to branch\n";
#endif
				branch[j] = Block(current_block->first, current_block->second.data(), BYTES_PER_BLOCK);
				current_block = stash.erase(current_block);
				goto RESTART;
			}
		}
		++current_block;
	}

	// if we are putting a new block
	uint16_t rand_leaf_id = oram_random(generator);
	uint16_t idx = find_intersection_bucket(position_map[requested_block_id], rand_leaf_id);
	position_map[requested_block_id] = rand_leaf_id;
	for (long j = (idx + 1) * BLOCKS_PER_BUCKET - 1; j >= 0; --j) {
		if (branch[j].get_block_id() == 0) {
			branch[j] = Block(requested_block_id, value_buffer->data(), BYTES_PER_BLOCK);
			return;
		}
	}
	memcpy(stash[requested_block_id].data(), value_buffer->data(), BYTES_PER_BLOCK);
}

uint16_t SingleClient::find_intersection_bucket(uint16_t leaf_id_1, uint16_t leaf_id_2)
{
	// returns the index (0) of the lowest intersecting bucket
	uint16_t idx = branch.size() / BLOCKS_PER_BUCKET - 1;
	--leaf_id_1;
	--leaf_id_2;
	while (leaf_id_1 != leaf_id_2) {
		leaf_id_1 >>= 1;
		leaf_id_2 >>= 1;
		--idx;
	}
	return idx;
}

int SingleClient::send_branch()
{
	for (uint32_t i = 0; i < branch.size() - 1; ++i) {
		branch[i].encrypt();
		if (send(socket_fd, branch[i].get_encrypted_data(), BLOCK_ID_SIZE + BYTES_PER_BLOCK, MSG_MORE) != BLOCK_ID_SIZE + BYTES_PER_BLOCK) {
			perror("send: failed");
			return -1;
		}
	}
	branch.back().encrypt();
	if (send(socket_fd, branch.back().get_encrypted_data(), BLOCK_ID_SIZE + BYTES_PER_BLOCK, 0) != BLOCK_ID_SIZE + BYTES_PER_BLOCK) {
		perror("send: failed");
		return -1;
	}
#ifdef DEBUG
	std::cout << "single_client: sent " << branch.size() << " updated blocks\n";
#endif
	return 0;
}

void SingleClient::store_state()
{
	int state_fd = open(".oram_state", O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (state_fd != -1) {
		unsigned int size;

		size = key_to_block_id.size();
		write(state_fd, &size, sizeof(size));
		for (const auto &pair : key_to_block_id) {
			unsigned int string_size = pair.first.size();
			write(state_fd, &string_size, sizeof(string_size));
			write(state_fd, pair.first.data(), string_size);
			write(state_fd, &pair.second, sizeof(pair.second));
		}

		size = position_map.size();
		write(state_fd, &size, sizeof(size));
		for (const auto &pair : position_map) {
			write(state_fd, &pair.first, sizeof(pair.first));
			write(state_fd, &pair.second, sizeof(pair.second));
		}

		size = stash.size();
		write(state_fd, &size, sizeof(size));
		for (const auto &pair : stash) {
			write(state_fd, &pair.first, sizeof(pair.first));
			write(state_fd, pair.second.data(), BYTES_PER_BLOCK);
		}

		close(state_fd);
	} else {
		std::cerr << "single_client: failed to open the output file\n";
	}
}
