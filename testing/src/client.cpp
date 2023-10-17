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
}

PathOramClient::~PathOramClient()
{

}

int PathOramClient::put(const std::string &key_name, const std::vector<uint8_t> &value)
{
	return 0;
}

// TODO: implement first
// TODO: replace all string data values to vector
int PathOramClient::get(const std::string &key_name, std::vector<uint8_t> &value)
{
	std::unique_ptr<std::vector<Block>> branch;
	if ( (branch = fetch_data(position_map[key_name])) == nullptr ) {
		std::cerr << "fetch_data: failed" << std::endl;
		exit(EXIT_FAILURE);
	}

	// decrypt branch if encrypted

	// perform swaps

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

std::unique_ptr<std::vector<Block>> PathOramClient::fetch_data(uint32_t leaf_id)
{
	/*
	BRANCH METADATA:
	uint32_t num_buckets	// can infer num_blocks = num_buckets * BLOCKS_PER_BUCKET
	uint32_t block_size[0]
	uint32_t block_size[1]
	uint32_t block_size[2]
	etc...
	block_size[0] block_data[0]	// block_data => leaf_id (uint32_t) + block_idx (uint32_t) + data (block_size - 8)
	block_size[1] block_data[1]
	block_size[2] block_data[2]
	etc...
	*/

	if (send(socket_fd, &leaf_id, 4, 0) != 4) {
		std::cerr << "send: failed" << std::endl;
		return nullptr;
	}

	uint32_t num_buckets;
	if (recv(socket_fd, &num_buckets, 4, 0) != 4) {
		std::cerr << "recv: failed" << std::endl;
		return nullptr;
	}

	uint32_t *block_size = (uint32_t *) malloc(4 * num_buckets * BLOCKS_PER_BUCKET);
	uint32_t max_block_size = 0;
	for (uint32_t i = 0; i < num_buckets * BLOCKS_PER_BUCKET; ++i) {
		if (recv(socket_fd, block_size + i, 4, 0) != 4) {
			std::cerr << "recv: failed" << std::endl;
			free(block_size);
			return nullptr;
		}
		if (block_size[i] > max_block_size)
			max_block_size = block_size[i];
	}

	std::unique_ptr<std::vector<Block>> branch;
	uint8_t *data_buffer = (uint8_t *) malloc(max_block_size);
	for (uint32_t i = 0; i < num_buckets * BLOCKS_PER_BUCKET; ++i) {
		if (recv(socket_fd, data_buffer, block_size[i], 0) != block_size[i]) {
			std::cerr << "recv: failed" << std::endl;
			free(block_size);
			free(data_buffer);
			return nullptr;
		}
		uint32_t leaf_id = 0;
		leaf_id |= static_cast<uint32_t>(data_buffer[0]) << 24;
		leaf_id |= static_cast<uint32_t>(data_buffer[1]) << 16;
		leaf_id |= static_cast<uint32_t>(data_buffer[2]) << 8;
		leaf_id |= static_cast<uint32_t>(data_buffer[3]);
		branch->push_back(Block(leaf_id, i % BLOCKS_PER_BUCKET, std::vector<uint8_t> (data_buffer + 8, data_buffer + block_size[i])));
		
	}

	return branch;
}
