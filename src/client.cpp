/*
class PathOramClient {
public:
	PathOramClient(const std::string &server_ip);
	~PathOramClient();

	int put(const std::string &key_name, const std::string &value);
	int get(const std::string &key_name, std::string &value);
	int read_range(const std::string &begin_key_name, const std::string &end_key_name);
	int clear_range(const std::string &begin_key_name, const std::string &end_key_name);

private:
	
};
*/

#include "client.h"
#include "bucket.h"
#include "block.h"
#include <algorithm>

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

int PathOramClient::put(const std::string &key_name, const std::vector<uint8_t> &value)
{
	return 0;
}

// TODO: implement first
// TODO: replace all vector data values to std::ARRAY
int PathOramClient::get(const std::string &key_name, std::array<uint8_t, BYTES_PER_BLOCK> &value)
{
	if (position_map.find(key_name) == position_map.end()) {
		std::cerr << "key is not in position map\n";
		return -1;
	}

	stash = fetch_branch(position_map[key_name]);
	if (stash.size() == 0) {
		std::cerr << "fetch_branch: failed" << std::endl;
		exit(EXIT_FAILURE);
	}

	// decrypt branch if encrypted

	// perform swaps
	for (uint32_t i = 0; i < stash.size(); ++i) {
		uint32_t current_leaf_id = stash[i].get_leaf_id();
		if (current_leaf_id == 0)
			continue;
		uint32_t requested_leaf_id = position_map[key_name];

		if (current_leaf_id == requested_leaf_id) {
			std::copy(stash[i].get_data().begin(), stash[i].get_data().end(), value.begin());
			uint32_t rand_leaf_id = 0; // TODO: set up randomness
			uint32_t idx = find_intersection_bucket(current_leaf_id, rand_leaf_id);
			long j;
			for (j = (idx + 1) * BLOCKS_PER_BUCKET - 1; j >= 0; --j) {
				if (stash[j].get_leaf_id() == 0) {
					swap_blocks(stash[i], stash[j]);
					break;
				}
			}
			if (j == -1) {
				// store in stash
				// free branch[i]
			}
			goto ENCRYPT;
		}

		uint32_t idx = find_intersection_bucket(current_leaf_id, requested_leaf_id);
		for (uint32_t j = idx * BLOCKS_PER_BUCKET; j < (idx + 1) * BLOCKS_PER_BUCKET; ++j) {
			if (stash[j].get_leaf_id() == 0) {
				swap_blocks(stash[i], stash[j]);
				break;
			}
		}
	}

	ENCRYPT:
	// encrypt branch

	// send branch back

	return 0;
}

int PathOramClient::read_range(const std::string &begin_key_name, const std::string &end_key_name)
{
	return 0;
}

int PathOramClient::clear_range(const std::string &begin_key_name, const std::string &end_key_name)
{
	return 0;
}

std::vector<Block> PathOramClient::fetch_branch(uint32_t leaf_id)
{
	/*
	BRANCH METADATA:
	uint32_t num_buckets	// can infer num_blocks = num_buckets * BLOCKS_PER_BUCKET
	uint32_t leaf_ids[0]
	uint32_t leaf_ids[1]
	...
	Block data_buffer[0]
	Block data_buffer[1]
	...
	*/

	if (send(socket_fd, &leaf_id, 4, 0) != 4) {
		std::cerr << "send: failed" << std::endl;
		return std::vector<Block> ();
	}
	std::cout << "client: sent leaf_id = " << leaf_id << '\n';

	uint32_t num_buckets;
	if (recv(socket_fd, &num_buckets, 4, 0) != 4) {
		std::cerr << "recv: failed" << std::endl;
		return std::vector<Block> ();
	}
	std::cout << "client: num_buckets = " << num_buckets << '\n';

	std::vector<uint32_t> leaf_ids (num_buckets * BLOCKS_PER_BUCKET, 0);
	if (recv(socket_fd, leaf_ids.data(), 4 * num_buckets * BLOCKS_PER_BUCKET, 0) != 4 * num_buckets * BLOCKS_PER_BUCKET) {
		std::cerr << "recv: failed" << std::endl;
		return std::vector<Block> ();
	}
	std::cout << "client: leaf_ids =";
	for (uint32_t id : leaf_ids)
		std::cout << ' ' << id;
	std::cout << '\n';

	std::vector<Block> branch (num_buckets * BLOCKS_PER_BUCKET);
	std::array<uint8_t, BYTES_PER_BLOCK> data_buffer;
	for (uint32_t i = 0; i < num_buckets * BLOCKS_PER_BUCKET; ++i) {
		data_buffer.fill(0);
		if (recv(socket_fd, data_buffer.data(), BYTES_PER_BLOCK, 0) != BYTES_PER_BLOCK) {
			std::cerr << "recv: failed" << std::endl;
			return std::vector<Block> ();
		}
		branch[i] = Block(leaf_ids[i], i % BLOCKS_PER_BUCKET, data_buffer);
	}

	return branch;
}

uint32_t PathOramClient::find_intersection_bucket(uint32_t leaf_id_1, uint32_t leaf_id_2)
{
	// returns the index of the lowest intersecting bucket
	uint32_t idx = stash.size() / BLOCKS_PER_BUCKET - 1;
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

int PathOramClient::send_branch(std::vector<Block> branch)
{
	return 0;
}
