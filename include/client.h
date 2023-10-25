#ifndef PORAM_CLIENT_H
#define PORAM_CLIENT_H

#include "block.h"
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <sys/socket.h>
#include <arpa/inet.h>

class PathOramClient {
public:
/*
	static PathOramClient *get_instance();
	PathOramClient() = delete;
	PathOramClient(const PathOramClient&) = delete;
	PathOramClient& operator=(const PathOramClient&) = delete;
*/
	PathOramClient(const std::string &server_ip = "127.0.0.1", const int port = 8080);
	~PathOramClient();

	int put(const std::string &key_name, const std::vector<uint8_t> &value);
	int get(const std::string &key_name, std::array<uint8_t, BYTES_PER_BLOCK> &value);
	int read_range(const std::string &begin_key_name, const std::string &end_key_name);
	int clear_range(const std::string &begin_key_name, const std::string &end_key_name);

private:
/*
	PathOramClient(const std::string &server_ip = "127.0.0.1", const int port = 8080);
	static PathOramClient *instance;
*/
	std::vector<Block> fetch_branch(uint32_t leaf_id);
	int send_branch(std::unique_ptr<std::vector<Block>> branch);
	uint32_t find_intersection_bucket(uint32_t leaf_id_1, uint32_t leaf_id_2);
	void swap_blocks(Block &block1, Block &block2);

	int socket_fd;
	std::map<std::string, uint32_t> position_map;
	std::vector<Block> stash;
};

enum Operation {
	READ = 1,
	WRITE,
	READ_RANGE,
	CLEAR_RANGE
};

#endif /* PORAM_CLIENT_H */
