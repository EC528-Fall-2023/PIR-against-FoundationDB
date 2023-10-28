#ifndef PORAM_CLIENT_H
#define PORAM_CLIENT_H

#include "block.h"
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <random>
#include <sys/socket.h>
#include <arpa/inet.h>

enum Operation {
	READ,
	WRITE
};

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

//	int put(const std::string &key_name, const std::vector<uint8_t> &value);
	int get(const std::string &key_name, std::array<uint8_t, BYTES_PER_BLOCK> &value);
//	int read_range(const std::string &begin_key_name, const std::string &end_key_name);
//	int clear_range(const std::string &begin_key_name, const std::string &end_key_name);

private:
/*
	PathOramClient(const std::string &server_ip = "127.0.0.1", const int port = 8080);
	static PathOramClient *instance;
*/
	int fetch_branch(uint16_t leaf_id);
	void traverse_branch(uint16_t requested_leaf_id, uint16_t rand_leaf_id, enum Operation op, std::array<uint8_t, BYTES_PER_BLOCK> &value);
	uint16_t find_intersection_bucket(uint16_t leaf_id_1, uint16_t leaf_id_2);
	void swap_blocks(Block &block1, Block &block2);
	int send_branch();

	int socket_fd;
	std::map<std::string, uint16_t> position_map;
	std::vector<Block> branch;
	std::vector<Block> stash;
};

#endif /* PORAM_CLIENT_H */
