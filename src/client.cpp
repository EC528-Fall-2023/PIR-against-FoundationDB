#include "client.h"
#include "bucket.h"
#include "block.h"
#include <algorithm>

static std::random_device generator;
static std::uniform_int_distribution<int> oram_random(1, 32768); // [1, 2^15]

PathOramClient::PathOramClient(const std::string &server_ip, const int port)
{
	if ( (socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		printf("socket: failed\n");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
		printf("inet_pton: failed\n");
		//close(socket_fd);
		exit(EXIT_FAILURE);
	}

	int status;
	if ( (status = connect(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr))) < 0 ) {
		printf("connect: failed\n");
		//close(socket_fd);
		exit(EXIT_FAILURE);
	}
	std::cout << "client: connection established\n";
}

PathOramClient::~PathOramClient()
{
	close(socket_fd);
}
/*
int PathOramClient::put(const std::string &key_name, const std::vector<uint8_t> &value)
{
	return 0;
}
*/
// TODO: implement first
int PathOramClient::get(const std::string &key_name, std::array<uint8_t, BYTES_PER_BLOCK> &value)
{
	position_map[key_name] = 1;
	if (position_map.find(key_name) == position_map.end()) {
		std::cerr << "key is not in position map\n";
		return -1;
	}

	uint16_t requested_leaf_id = position_map[key_name];
	uint16_t rand_leaf_id = oram_random(generator);
	position_map[key_name] = rand_leaf_id;

	if (fetch_branch(requested_leaf_id) == -1) {
		std::cerr << "client: fetch_branch: failed" << std::endl;
		exit(EXIT_FAILURE);
	}

	// decrypt branch if encrypted

	// perform swaps
	traverse_branch(requested_leaf_id, rand_leaf_id, READ, value);

	// encrypt branch

	// send branch back
	if (send_branch() == -1) {
		std::cerr << "client: send_branch: failed\n";
		exit(EXIT_FAILURE);
	}

	return 0;
}
/*
int PathOramClient::read_range(const std::string &begin_key_name, const std::string &end_key_name)
{
	return 0;
}

int PathOramClient::clear_range(const std::string &begin_key_name, const std::string &end_key_name)
{
	return 0;
}
*/
int PathOramClient::fetch_branch(uint16_t leaf_id)
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
		if (i % 3 == 0)
			std::cout << " |";
		std::cout << ' ' << leaf_ids[i];
	}
	std::cout << '\n';

	std::array<uint8_t, BYTES_PER_BLOCK> data_buffer;
	for (uint16_t i = 0; i < num_blocks; ++i) {
		data_buffer.fill(0);
		if (recv(socket_fd, data_buffer.data(), BYTES_PER_BLOCK, 0) != BYTES_PER_BLOCK) {
			std::cerr << "client: recv: failed" << std::endl;
			return -1;
		}
		branch[i] = Block(leaf_ids[i], data_buffer);
	}
	std::cout << "client: recv " << num_blocks << " blocks\n";

	return 0;
}

inline void PathOramClient::traverse_branch(uint16_t requested_leaf_id, uint16_t rand_leaf_id, enum Operation op, std::array<uint8_t, BYTES_PER_BLOCK> &value)
{
	for (Block &current_block : branch) {
		uint16_t current_leaf_id = current_block.get_leaf_id();
		if (current_leaf_id == 0)
			continue;

		if (current_leaf_id == requested_leaf_id) {
			if (op == READ)
				std::copy(current_block.get_data().begin(), current_block.get_data().end(), value.begin());
			else // if (op == WRITE)
				std::copy(value.begin(), value.end(), current_block.get_data().begin());
			uint16_t idx = find_intersection_bucket(current_leaf_id, rand_leaf_id);
			std::cout << "client: found leaf_id " << requested_leaf_id << ", randomized to id " << rand_leaf_id << ", bucket idx " << idx << '\n';
			long j;
			for (j = (idx + 1) * BLOCKS_PER_BUCKET - 1; j >= 0; --j) {
				if (branch[j].get_leaf_id() == 0) {
					swap_blocks(current_block, branch[j]);
					return;
				}
			}
			if (j == -1) {
				// store in branch
				// free branch[i]
			}
		} else {
			uint16_t idx = find_intersection_bucket(current_leaf_id, requested_leaf_id);
			long j;
			for (j = idx * BLOCKS_PER_BUCKET; j < (idx + 1) * BLOCKS_PER_BUCKET; ++j) {
				if (branch[j].get_leaf_id() == 0) {
					swap_blocks(current_block, branch[j]);
					break;
				}
			}
			if (j == (idx + 1) * BLOCKS_PER_BUCKET) {
				// store in stash
				// free branch[i]
			}
		}
	}
}

uint16_t PathOramClient::find_intersection_bucket(uint16_t leaf_id_1, uint16_t leaf_id_2)
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

inline void PathOramClient::swap_blocks(Block &block1, Block &block2)
{
	Block temp = block1;
	block1.set_leaf_id(block2.get_leaf_id());
	block1.set_data(block2.get_data());
	block2.set_leaf_id(temp.get_leaf_id());
	block2.set_data(temp.get_data());
}

int PathOramClient::send_branch()
{
	for (Block &block : branch) {
		uint16_t leaf_id = block.get_leaf_id();
		if (send(socket_fd, &leaf_id, sizeof(leaf_id), 0) != sizeof(leaf_id)) {
			std::cerr << "send: failed\n";
			return -1;
		}
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
