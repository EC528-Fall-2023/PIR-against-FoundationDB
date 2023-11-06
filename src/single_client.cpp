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
		printf("client: connect: failed\n");
		exit(EXIT_FAILURE);
	}
	std::cout << "client: connection to " << server_ip << " established\n";

	if (access(".oram_state", F_OK) == 0) {
		int state_fd;
		if ( (state_fd = open(".oram_state", O_RDONLY | O_CREAT, 0666)) == -1) {
			std::cerr << "client: open state: failed " << errno << '\n';
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
	std::cout << "client: disconnected\n";
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
		std::cerr << "client: send: failed\n";
		close(socket_fd);
		return -1;
	}
	std::cout << "client: request_id = " << request_id << '\n';

	if (fetch_branch(position_map[requested_block_id]) == -1) {
		std::cerr << "client: fetch_branch: failed" << std::endl;
		close(socket_fd);
		return -1;
	}

	// decrypt branch if encrypted

	// perform swaps
	std::array<uint8_t, BYTES_PER_BLOCK> value_buffer = value;
	traverse_branch(requested_block_id, WRITE, value_buffer);

	// encrypt branch

	// send branch back
	if (send_branch() == -1) {
		std::cerr << "client: send_branch: failed\n";
		close(socket_fd);
		return -1;
	}

	// confirm the operation completed
	uint16_t confirmation_id = 0;
	if (recv(socket_fd, &confirmation_id, sizeof(confirmation_id), 0) != sizeof(confirmation_id) || confirmation_id != request_id) {
		std::cerr << "client: recv: failed\n";
		close(socket_fd);
		return -1;
	}
	std::cout << "client: PUT SUCCESS\n";
	branch.clear();
	store_state();

	return 0;
}

// TODO: implement first
int SingleClient::get(const std::string &key_name, std::array<uint8_t, BYTES_PER_BLOCK> &value)
{
	if (key_to_block_id.find(key_name) == key_to_block_id.end()) {
		std::cerr << "key is not in position map\n";
		return -1;
	}

	uint16_t requested_block_id = key_to_block_id[key_name];

	uint16_t request_id = oram_random(generator);
	if (send(socket_fd, &request_id, sizeof(request_id), 0) != sizeof(request_id)) {
		std::cerr << "client: send: failed\n";
		close(socket_fd);
		return -1;
	}
	std::cout << "client: request_id = " << request_id << '\n';

	if (fetch_branch(position_map[requested_block_id]) == -1) {
		std::cerr << "client: fetch_branch: failed" << std::endl;
		close(socket_fd);
		return -1;
	}

	// decrypt branch if encrypted

	// perform swaps
	std::array<uint8_t, BYTES_PER_BLOCK> value_buffer = {0};
	traverse_branch(requested_block_id, READ, value_buffer);

	// encrypt branch

	// send branch back
	if (send_branch() == -1) {
		std::cerr << "client: send_branch: failed\n";
		close(socket_fd);
		return -1;
	}

	// confirm the operation completed
	uint16_t confirmation_id = 0;
	if (recv(socket_fd, &confirmation_id, sizeof(confirmation_id), 0) != sizeof(confirmation_id) || confirmation_id != request_id) {
		std::cerr << "client: recv: failed\n";
		close(socket_fd);
		return -1;
	}

	std::cout << "client: GET SUCCESS\n";
	memcpy(value.data(), value_buffer.data(), BYTES_PER_BLOCK);
	branch.clear();
	store_state();

	return 0;
}

int SingleClient::read_range(const std::string &begin_key_name, const std::string &end_key_name, std::vector<std::array<uint8_t, BYTES_PER_BLOCK>> &data)
{
	for (auto map_iterator = key_to_block_id.lower_bound(begin_key_name); map_iterator != key_to_block_id.upper_bound(end_key_name); ++map_iterator) {
		std::array<uint8_t, BYTES_PER_BLOCK> data_buffer;
		if (get(map_iterator->first, data_buffer) != 0) {
			std::cerr << "read_range: failed. " << map_iterator->first << '\n';
			return -1;
		}
		data.push_back(data_buffer);
	}
	return 0;
}
/*
int SingleClient::clear_range(const std::string &begin_key_name, const std::string &end_key_name)
{
	for (auto map_iterator = key_to_block_id.lower_bound(begin_key_name); map_iterator != key_to_block_id.upper_bound(end_key_name); ++map_iterator) {
		
	}
	return 0;
}
*/
int SingleClient::fetch_branch(uint16_t leaf_id)
{
	if (send(socket_fd, &leaf_id, sizeof(leaf_id), 0) != sizeof(leaf_id)) {
		std::cerr << "client: send: failed" << std::endl;
		return -1;
	}
	std::cout << "client: sent leaf_id = " << leaf_id << '\n';

	uint16_t num_blocks;
	if (recv(socket_fd, &num_blocks, sizeof(num_blocks), 0) != sizeof(num_blocks)) {
		std::cerr << "client: recv: failed" << std::endl;
		return -1;
	}
	branch.resize(num_blocks);
	std::cout << "client: num_blocks = " << num_blocks << '\n';

	std::vector<uint16_t> leaf_ids (num_blocks, 0);
	if (recv(socket_fd, leaf_ids.data(), sizeof(uint16_t) * num_blocks, 0) != (long int) sizeof(uint16_t) * num_blocks) {
		std::cerr << "client: recv: failed" << std::endl;
		return -1;
	}
	std::cout << "client: leaf_ids =";
	for (int i = 0; i < num_blocks; ++i) {
		if (i % BLOCKS_PER_BUCKET == 0)
			std::cout << " |";
		std::cout << ' ' << leaf_ids[i];
	}
	std::cout << '\n';

	std::array<uint8_t, BYTES_PER_BLOCK> data_buffer;
	for (uint16_t i = 0; i < num_blocks; ++i) {
		if (recv(socket_fd, data_buffer.data(), BYTES_PER_BLOCK, 0) != BYTES_PER_BLOCK) {
			std::cerr << "client: recv: failed" << std::endl;
			return -1;
		}
		branch[i] = Block(leaf_ids[i], data_buffer);
	}
	std::cout << "client: recv " << num_blocks << " blocks\n";

	return 0;
}

void SingleClient::traverse_branch(uint16_t requested_block_id, enum Operation op, std::array<uint8_t, BYTES_PER_BLOCK> &value_buffer)
{
	for (Block &current_block : branch) {
		if (current_block.get_block_id() == 0)
			continue;

		if (current_block.get_block_id() == requested_block_id) {
			if (op == READ)
				std::copy(current_block.get_data().begin(), current_block.get_data().end(), value_buffer.begin());
			else // if (op == WRITE)
				std::copy(value_buffer.begin(), value_buffer.end(), current_block.get_data().begin());
			uint16_t rand_leaf_id = oram_random(generator);
			uint16_t idx = find_intersection_bucket(position_map[requested_block_id], rand_leaf_id);
			position_map[requested_block_id] = rand_leaf_id;
			std::cout << "client: found leaf_id " << requested_block_id << ", randomized to leaf_id " << rand_leaf_id << ", bucket idx " << idx << '\n';
			for (long j = (idx + 1) * BLOCKS_PER_BUCKET - 1; j >= 0; --j) {
				if (branch[j].get_block_id() == 0) {
					swap_blocks(current_block, branch[j]);
					return;
				}
			}
			// store in stash
			// free current block

			stash[current_block.get_block_id()] = current_block.get_data();
			std::cout << "wrote block " << current_block.get_block_id() << " to stash\n";
			current_block.set_block_id(0);
			current_block.set_random_data();
			return;
		}
		uint16_t idx = find_intersection_bucket(position_map[requested_block_id], position_map[current_block.get_block_id()]);
		long j;
		for (j = idx * BLOCKS_PER_BUCKET; j < (idx + 1) * BLOCKS_PER_BUCKET; ++j) {
			if (branch[j].get_block_id() == 0) {
				swap_blocks(current_block, branch[j]);
				break;
			}
		}
		if (j == (idx + 1) * BLOCKS_PER_BUCKET) {
			// store in stash
			// free current block

			stash[current_block.get_block_id()] = current_block.get_data();
			std::cout << "wrote block " << current_block.get_block_id() << " to stash\n";
			current_block.set_block_id(0);
			current_block.set_random_data();
		}
	}

	// look for requested block in the stash
	for (auto current_block = stash.begin(); current_block != stash.end(); ) {
		RESTART:
		if (current_block->first == requested_block_id) {
			if (op == READ)
				std::copy(current_block->second.begin(), current_block->second.end(), value_buffer.begin());
			else // if (op == WRITE)
				std::copy(value_buffer.begin(), value_buffer.end(), current_block->second.begin());
			uint16_t rand_leaf_id = oram_random(generator);
			uint16_t idx = find_intersection_bucket(position_map[requested_block_id], rand_leaf_id);
			position_map[requested_block_id] = rand_leaf_id;
			std::cout << "client: found leaf_id in stash" << requested_block_id << ", randomized to leaf_id " << rand_leaf_id << ", bucket idx " << idx << '\n';
			for (long j = (idx + 1) * BLOCKS_PER_BUCKET - 1; j >= 0; --j) {
				if (branch[j].get_block_id() == 0) {
					std::cout << "client: moved block " << current_block->first << " from stash to branch\n";
					branch[j] = Block(current_block->first, current_block->second);
					stash.erase(current_block);
					break;
				}
			}
			return;
		}
		uint16_t idx = find_intersection_bucket(position_map[requested_block_id], position_map[current_block->first]);
		for (long j = idx * BLOCKS_PER_BUCKET; j < (idx + 1) * BLOCKS_PER_BUCKET; ++j) {
			if (branch[j].get_block_id() == 0) {
				std::cout << "client: moved block " << current_block->first << " from stash to branch\n";
				branch[j] = Block(current_block->first, current_block->second);
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
			branch[j] = Block(requested_block_id, value_buffer);
			return;
		}
	}
	stash[requested_block_id] = value_buffer;
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

inline void SingleClient::swap_blocks(Block &block1, Block &block2)
{
	Block temp = block1;
	block1.set_block_id(block2.get_block_id());
	block1.set_data(block2.get_data());
	block2.set_block_id(temp.get_block_id());
	block2.set_data(temp.get_data());
}

int SingleClient::send_branch()
{
	std::vector<uint16_t> leaf_ids (branch.size(), 0);
	for (unsigned long i = 0; i < leaf_ids.size(); ++i) {// changed to branch
		leaf_ids[i] = branch[i].get_block_id();
	}

	if (send(socket_fd, leaf_ids.data(), sizeof(uint16_t) * leaf_ids.size(), 0) != (long int) (sizeof(uint16_t) * leaf_ids.size())) {
		std::cerr << "send: failed\n";
		return -1;
	}
	std::cout << "client: sent " << branch.size() << " updated leaf_ids\n";


	for (Block &block : branch) {
		if (send(socket_fd, block.get_data().data(), BYTES_PER_BLOCK, 0) != BYTES_PER_BLOCK) {
			std::cerr << "send: failed\n";
			return -1;
		}
	}
	std::cout << "client: sent " << branch.size() << " updated blocks\n";
	return 0;
}
/*
Block PathOramClient::fetch_from_stash(uint16_t leaf_id) {
    std::vector<Block>::iterator it = stash.begin(); // use iterator 
    
    while (it != stash.end()) {
        if (it->get_block_id() == leaf_id) {
            
            Block fetched_block = *it; // Block being found in the stash
            stash.erase(it); // Remove the block from the stash- since its no longer in stash

            return fetched_block;
        }
        ++it;
    }

    return Block(0, {}); // return an empty block if not found
}
*/


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
		std::cerr << "client: failed to open the output file\n";
	}
}
