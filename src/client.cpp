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
using namespace std;

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
int PathOramClient::get(const std::string &key_name, std::vector<uint8_t> &value)
{
	std::unique_ptr<std::vector<Block>> branch;
	if ( (branch = fetch_branch(position_map[key_name])) == nullptr ) {
		std::cerr << "fetch_branch: failed" << std::endl;
		exit(EXIT_FAILURE);
	}

	// decrypt branch if encrypted

	// perform swaps
	for (uint32_t i = 0; i < branch->size(); ++i) {
		if ((*branch)[i].get_leaf_id() == 0)
			continue;
		
	}

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

std::unique_ptr<std::vector<Block>> PathOramClient::fetch_branch(uint32_t leaf_id)
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
		return nullptr;
	}
	std::cout << "client: sent leaf_id = " << leaf_id << '\n';

	uint32_t num_buckets;
	if (recv(socket_fd, &num_buckets, 4, 0) != 4) {
		std::cerr << "recv: failed" << std::endl;
		return nullptr;
	}
	std::cout << "client: num_buckets = " << num_buckets << '\n';

	std::vector<uint32_t> leaf_ids (num_buckets * BLOCKS_PER_BUCKET, 0);
	if (recv(socket_fd, leaf_ids.data(), 4 * num_buckets * BLOCKS_PER_BUCKET, 0) != 4 * num_buckets * BLOCKS_PER_BUCKET) {
		std::cerr << "recv: failed" << std::endl;
		return nullptr;
	}
	std::cout << "client: leaf_ids =";
	for (uint32_t id : leaf_ids)
		std::cout << ' ' << id;
	std::cout << '\n';

	std::unique_ptr<std::vector<Block>> branch (new std::vector<Block> (num_buckets * BLOCKS_PER_BUCKET));
	std::array<uint8_t, BYTES_PER_BLOCK> data_buffer;
	for (uint32_t i = 0; i < num_buckets * BLOCKS_PER_BUCKET; ++i) {
		data_buffer.fill(0);
		if (recv(socket_fd, data_buffer.data(), BYTES_PER_BLOCK, 0) != BYTES_PER_BLOCK) {
			std::cerr << "recv: failed" << std::endl;
			return nullptr;
		}
		(*branch)[i] = Block(leaf_ids[i], i % BLOCKS_PER_BUCKET, data_buffer);
	}

	return branch;
}
