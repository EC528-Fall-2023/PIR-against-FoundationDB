#include "single_client.h"

#ifdef DEBUG
	#include <error.h>
	#define ERROR(function) error_at_line(0, errno, __FILE__, __LINE__, "%s: %s failed", __func__, function);
#else
	#define ERROR(function)
#endif
static std::uniform_int_distribution<blkid_t> oram_random(1, 1 << (TREE_LEVELS - 1));
#ifdef SEEDED_RANDOM
std::mt19937 generator(42);
#else
static std::random_device generator;
#endif
#define RAND() oram_random(generator)

SingleClient::SingleClient() {}

int SingleClient::initialize(const std::string &server_ip, const int port, int (*custom_init)())
{
	if (access(".oram_enc", F_OK) != 0) {
		ERROR("access");
		return -1;
	}
	int encryption_fd;
	if ( (encryption_fd = open(".oram_enc", O_RDONLY)) == -1 ) {
		ERROR("open");
		return -1;
	}
	if (read(encryption_fd, enc_key, 32) != 32 || lseek(encryption_fd, 1, SEEK_CUR) != 33 || read(encryption_fd, enc_iv, 16) != 16) {
		ERROR("read encryption keys");
		return -1;
	}

	if ( (socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		ERROR("socket");
		return -1;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
		ERROR("inet_pton");
		close(socket_fd);
		return -1;
	}

	if (connect(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0 ) {
		ERROR("connect");
		return -1;
	}
#ifdef DEBUG
	std::cout << "single_client: connection to " << server_ip << ':' << port << " established\n";
#endif

	uint8_t fdb_is_initialized;
	if (recv(socket_fd, &fdb_is_initialized, sizeof(fdb_is_initialized), 0) != sizeof(fdb_is_initialized)) {
		ERROR("send");
		return -1;
	}

	if (fdb_is_initialized)
		return 0;

	if (custom_init == nullptr) {
		uint8_t random_data[BYTES_PER_DATA];
		for (uint32_t i = 0; i < BLOCKS_PER_BUCKET * BUCKETS_PER_TREE - 1; ++i) {
			memset(random_data, 0, BYTES_PER_DATA);
			for (uint32_t j = 0; j < BYTES_PER_DATA; ++j) {
				random_data[j] = RAND();
			}
			Block block(0, random_data, BYTES_PER_DATA);
			if (block.encrypt(enc_key, enc_iv) == -1) {
				ERROR("encrypt");
				return -1;
			}
			if (send(socket_fd, block.get_encrypted_data(), BLOCK_SIZE, MSG_MORE) != BLOCK_SIZE) {
				ERROR("send");
				return -1;
			}
		}
		memset(random_data, 0, BYTES_PER_DATA);
		for (uint32_t j = 0; j < BYTES_PER_DATA; ++j) {
			random_data[j] = RAND();
		}
		Block block(0, random_data, BYTES_PER_DATA);
		if (block.encrypt(enc_key, enc_iv) == -1) {
			ERROR("encrypt");
			return -1;
		}
		if (send(socket_fd, block.get_encrypted_data(), BLOCK_SIZE, 0) != BLOCK_SIZE) {
			ERROR("send");
			return -1;
		}
	} else if (custom_init() != 0){
		ERROR("custom_init");
		return -1;
	}

	if (access(".oram_state", F_OK) == 0) {
		int state_fd;
		if ( (state_fd = open(".oram_state", O_RDONLY | O_CREAT, 0666)) == -1 ) {
			ERROR("open");
			return -1;
		}

		std::string key;
		blkid_t block_id;
		blkid_t leaf_id;
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
			std::array<uint8_t, BYTES_PER_DATA> temp;
			read(state_fd, &block_id, sizeof(block_id));
			read(state_fd, temp.data(), BYTES_PER_DATA);
			stash[block_id] = temp;
		}

		close(state_fd);
	}

	return 0;
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

int SingleClient::put(const std::string &key_name, const std::array<uint8_t, BYTES_PER_DATA> &value)
{
	// if key does not exist yet, add new entry
	if (key_to_block_id.find(key_name) == key_to_block_id.end()) {
		unsigned long int i;
		for (i = 1; i <= position_map.size(); ++i) {
			if (position_map.find(i) == position_map.end())
				break;
		}
		key_to_block_id[key_name] = i;
		position_map[i] = RAND();
	}

	blkid_t requested_block_id = key_to_block_id[key_name];
	
	blkid_t request_id = RAND();
	if (send(socket_fd, &request_id, sizeof(request_id), 0) != sizeof(request_id)) {
		ERROR("send");
		close(socket_fd);
		return -1;
	}
#ifdef DEBUG
	std::cout << "single_client: request_id = " << request_id << '\n';
#endif

	if (fetch_branch(position_map[requested_block_id]) == -1) {
		ERROR("fetch_branch");
		close(socket_fd);
		return -1;
	}

	// perform swaps
	std::array<uint8_t, BYTES_PER_DATA> value_buffer = value;
	traverse_branch(requested_block_id, WRITE, &value_buffer);
#ifdef DEBUG
	std::cout << "single_client: updated leaf_ids =";
	for (uint32_t i = 0; i < branch.size(); ++i) {
		if (i % BLOCKS_PER_BUCKET == 0)
			std::cout << " |";
		std::cout << ' ' << branch[i].get_block_id();
	}
	std::cout << '\n';
	std::cout << "single_client: recv " << branch.size() << " blocks\n";
#endif

	// send branch back
	if (send_branch() == -1) {
		ERROR("send_branch");
		close(socket_fd);
		return -1;
	}

	// confirm the operation completed
	blkid_t confirmation_id = 0;
	if (recv(socket_fd, &confirmation_id, sizeof(confirmation_id), 0) != sizeof(confirmation_id) || confirmation_id != request_id) {
		ERROR("recv");
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

int SingleClient::get(const std::string &key_name, std::array<uint8_t, BYTES_PER_DATA> &value)
{
	if (key_to_block_id.find(key_name) == key_to_block_id.end()) {
#ifdef DEBUG
		std::cout << "key is not in position map\n";
#endif
		return -1;
	}

	blkid_t requested_block_id = key_to_block_id[key_name];

	blkid_t request_id = RAND();
	if (send(socket_fd, &request_id, sizeof(request_id), 0) != sizeof(request_id)) {
		ERROR("send");
		close(socket_fd);
		return -1;
	}
#ifdef DEBUG
	std::cout << "single_client: request_id = " << request_id << '\n';
#endif

	if (fetch_branch(position_map[requested_block_id]) == -1) {
		ERROR("fetch_branch");
		close(socket_fd);
		return -1;
	}

	// perform swaps
	std::array<uint8_t, BYTES_PER_DATA> value_buffer = {0};
	traverse_branch(requested_block_id, READ, &value_buffer);
#ifdef DEBUG
	std::cout << "single_client: updated leaf_ids =";
	for (uint32_t i = 0; i < branch.size(); ++i) {
		if (i % BLOCKS_PER_BUCKET == 0)
			std::cout << " |";
		std::cout << ' ' << branch[i].get_block_id();
	}
	std::cout << '\n';
	std::cout << "single_client: recv " << branch.size() << " blocks\n";
#endif

	// send branch back
	if (send_branch() == -1) {
		ERROR("send_branch");
		close(socket_fd);
		return -1;
	}

	// confirm the operation completed
	blkid_t confirmation_id = 0;
	if (recv(socket_fd, &confirmation_id, sizeof(confirmation_id), 0) != sizeof(confirmation_id) || confirmation_id != request_id) {
		ERROR("recv");
		close(socket_fd);
		return -1;
	}

#ifdef DEBUG
	std::cout << "single_client: GET SUCCESS\n";
#endif
	store_state();
	memcpy(value.data(), value_buffer.data(), BYTES_PER_DATA);
	branch.clear();

	return 0;
}

int SingleClient::clear(const std::string &key_name)
{
	if (key_to_block_id.find(key_name) == key_to_block_id.end())
		return 0;

	blkid_t requested_block_id = key_to_block_id[key_name];
	
	blkid_t request_id = RAND();
	if (send(socket_fd, &request_id, sizeof(request_id), 0) != sizeof(request_id)) {
		ERROR("send");
		close(socket_fd);
		return -1;
	}
#ifdef DEBUG
	std::cout << "single_client: request_id = " << request_id << '\n';
#endif

	if (fetch_branch(position_map[requested_block_id]) == -1) {
		ERROR("fetch_branch");
		close(socket_fd);
		return -1;
	}

	// perform swaps
	traverse_branch(requested_block_id, CLEAR, nullptr);
#ifdef DEBUG
	std::cout << "single_client: updated leaf_ids =";
	for (uint32_t i = 0; i < branch.size(); ++i) {
		if (i % BLOCKS_PER_BUCKET == 0)
			std::cout << " |";
		std::cout << ' ' << branch[i].get_block_id();
	}
	std::cout << '\n';
	std::cout << "single_client: recv " << branch.size() << " blocks\n";
#endif

	// send branch back
	if (send_branch() == -1) {
		ERROR("send_branch");
		close(socket_fd);
		return -1;
	}

	// confirm the operation completed
	blkid_t confirmation_id = 0;
	if (recv(socket_fd, &confirmation_id, sizeof(confirmation_id), 0) != sizeof(confirmation_id) || confirmation_id != request_id) {
		ERROR("recv");
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

int SingleClient::read_range(const std::string &begin_key_name, const std::string &end_key_name, std::vector<std::array<uint8_t, BYTES_PER_DATA>> &data)
{
	for (auto map_iterator = key_to_block_id.lower_bound(begin_key_name); map_iterator != key_to_block_id.upper_bound(end_key_name); ++map_iterator) {
		std::array<uint8_t, BYTES_PER_DATA> data_buffer;
		if (get(map_iterator->first, data_buffer) != 0) {
			ERROR("get");
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
			ERROR("clear");
			return -1;
		}
	}
	return 0;
}

int SingleClient::fetch_branch(blkid_t leaf_id)
{
	if (send(socket_fd, &leaf_id, sizeof(leaf_id), 0) != sizeof(leaf_id)) {
		ERROR("send");
		return -1;
	}
#ifdef DEBUG
	std::cout << "single_client: sent leaf_id = " << leaf_id << '\n';
#endif

	blkid_t num_blocks;
	if (recv(socket_fd, &num_blocks, sizeof(num_blocks), 0) != sizeof(num_blocks)) {
		ERROR("recv");
		return -1;
	}
	branch.resize(num_blocks);
#ifdef DEBUG
	std::cout << "single_client: num_blocks = " << num_blocks << '\n';
#endif

	for (blkid_t i = 0; i < num_blocks; ++i) {
		if (recv(socket_fd, branch[i].get_encrypted_data(), BLOCK_SIZE, MSG_WAITALL) != BLOCK_SIZE) {
			ERROR("recv");
			return -1;
		}
		if (branch[i].decrypt(enc_key, enc_iv) == -1) {
			ERROR("decrypt");
			return -1;
		}
	}

#ifdef DEBUG
	std::cout << "single_client: leaf_ids =";
	for (int i = 0; i < num_blocks; ++i) {
		if (i % BLOCKS_PER_BUCKET == 0)
			std::cout << " |";
		std::cout << ' ' << branch[i].get_block_id();
	}
	std::cout << '\n';
	std::cout << "single_client: recv " << num_blocks << " blocks\n";
#endif

	return 0;
}

void SingleClient::traverse_branch(blkid_t requested_block_id, enum Operation op, std::array<uint8_t, BYTES_PER_DATA> *value_buffer)
{
	for (Block &current_block : branch) {
		if (current_block.get_block_id() == 0)
			continue;

		if (current_block.get_block_id() == requested_block_id) {
			if (op == READ)
				memcpy(value_buffer->data(), current_block.get_decrypted_data(), BYTES_PER_DATA);
			else if (op == WRITE)
				memcpy(current_block.get_decrypted_data(), value_buffer->data(), BYTES_PER_DATA);
			else if (op == CLEAR)
				current_block.set_block_id(0);

			blkid_t rand_leaf_id = RAND();
			blkid_t idx = find_intersection_bucket(position_map[requested_block_id], rand_leaf_id);
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

			memcpy(stash[current_block.get_block_id()].data(), current_block.get_decrypted_data(), BYTES_PER_DATA);
#ifdef DEBUG
			std::cout << "single_client: wrote block " << current_block.get_block_id() << " to stash\n";
#endif
			current_block.set_block_id(0);
			current_block.set_decrypted_random_data();
			return;
		}
		blkid_t idx = find_intersection_bucket(position_map[requested_block_id], position_map[current_block.get_block_id()]);
		long j;
		for (j = idx * BLOCKS_PER_BUCKET; j < (idx + 1) * BLOCKS_PER_BUCKET; ++j) {
			if (branch[j].get_block_id() == 0) {
#ifdef DEBUG
				std::cout << "single_client: swap block " << current_block.get_block_id() << " to branch idx " << j << '\n';
#endif
				current_block.swap(branch[j]);
				break;
			}
		}
		if (j == (idx + 1) * BLOCKS_PER_BUCKET) {
			// store in stash
			// free current block

			memcpy(stash[current_block.get_block_id()].data(), current_block.get_decrypted_data(), BYTES_PER_DATA);
#ifdef DEBUG
			std::cout << "single_client: wrote block " << current_block.get_block_id() << " to stash (not requested block)\n";
#endif
			current_block.set_block_id(0);
			current_block.set_decrypted_random_data();
		}
	}

	// look for requested block in the stash
	std::vector<blkid_t> keys_to_remove;
	for (auto current_block = stash.begin(); current_block != stash.end(); ) {
		RESTART:
		if (current_block->first == requested_block_id) {
			if (op == READ)
				memcpy(value_buffer->data(), current_block->second.data(), BYTES_PER_DATA);
			else if (op == WRITE)
				memcpy(current_block->second.data(), value_buffer->data(), BYTES_PER_DATA);
			else if (op == CLEAR) {
				stash.erase(current_block);
				for (blkid_t key : keys_to_remove) {
					stash.erase(key);
				}
				return;
			}
			blkid_t rand_leaf_id = RAND();
			blkid_t idx = find_intersection_bucket(position_map[requested_block_id], rand_leaf_id);
			position_map[requested_block_id] = rand_leaf_id;
#ifdef DEBUG
			std::cout << "single_client: found leaf_id in stash" << requested_block_id << ", randomized to leaf_id " << rand_leaf_id << ", bucket idx " << idx << '\n';
#endif
			for (long j = (idx + 1) * BLOCKS_PER_BUCKET - 1; j >= 0; --j) {
				if (branch[j].get_block_id() == 0) {
#ifdef DEBUG
					std::cout << "single_client: moved block " << current_block->first << " from stash to branch index " << j << '\n';
#endif
					branch[j] = Block(current_block->first, current_block->second.data(), BYTES_PER_DATA);
					keys_to_remove.push_back(current_block->first);
					//stash.erase(current_block);
					break;
				}
			}
			for (blkid_t key : keys_to_remove) {
				stash.erase(key);
			}
			return;
		}
		blkid_t idx = find_intersection_bucket(position_map[requested_block_id], position_map[current_block->first]);
		for (long j = idx * BLOCKS_PER_BUCKET; j < (idx + 1) * BLOCKS_PER_BUCKET; ++j) {
			if (branch[j].get_block_id() == 0) {
#ifdef DEBUG
				std::cout << "single_client: moved block " << current_block->first << " from stash to branch index " << j << '\n';
#endif
				branch[j] = Block(current_block->first, current_block->second.data(), BYTES_PER_DATA);
				keys_to_remove.push_back(current_block->first);
				//current_block = stash.erase(current_block);
				goto RESTART;
			}
		}
		++current_block;
	}
	for (blkid_t key : keys_to_remove) {
		stash.erase(key);
	}

	// if we are putting a new block
	blkid_t rand_leaf_id = RAND();
	blkid_t idx = find_intersection_bucket(position_map[requested_block_id], rand_leaf_id);
	position_map[requested_block_id] = rand_leaf_id;
	for (long j = (idx + 1) * BLOCKS_PER_BUCKET - 1; j >= 0; --j) {
		if (branch[j].get_block_id() == 0) {
			branch[j] = Block(requested_block_id, value_buffer->data(), BYTES_PER_DATA);
#ifdef DEBUG
			std::cout << "single_client: insert block " << requested_block_id << ", randomized to leaf_id " << rand_leaf_id << ", bucket idx " << idx << '\n';
#endif
			return;
		}
	}
	memcpy(stash[requested_block_id].data(), value_buffer->data(), BYTES_PER_DATA);
}

blkid_t SingleClient::find_intersection_bucket(blkid_t leaf_id_1, blkid_t leaf_id_2)
{
	// returns the index (0) of the lowest intersecting bucket
	blkid_t idx = TREE_LEVELS - 1;
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
		if (branch[i].encrypt(enc_key, enc_iv) == -1) {
			ERROR("encrypt");
			return -1;
		}
		if (send(socket_fd, branch[i].get_encrypted_data(), BLOCK_SIZE, MSG_MORE) != BLOCK_SIZE) {
			ERROR("send");
			return -1;
		}
	}
	if (branch.back().encrypt(enc_key, enc_iv) == -1) {
		ERROR("encrypt");
		return -1;
	}
	if (send(socket_fd, branch.back().get_encrypted_data(), BLOCK_SIZE, 0) != BLOCK_SIZE) {
		ERROR("send");
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
			write(state_fd, pair.second.data(), BYTES_PER_DATA);
		}

		close(state_fd);
	} else {
		ERROR("open");
	}
}
