#ifndef PORAM_SINGLE_CLIENT_H
#define PORAM_SINGLE_CLIENT_H

#include "block.h"
#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <random>
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>

enum Operation {
	READ,
	WRITE,
	CLEAR,
	READ_RANGE,
	CLEAR_RANGE
};

class SingleClient {
public:
	SingleClient();
	~SingleClient();

	int initialize(const std::string &server_ip = "127.0.0.1", const int port = 8080, int (*custom_init)() = nullptr);
	int shutdown();
	int put(const std::string &key_name, const std::array<uint8_t, BYTES_PER_DATA> &value);
	int get(const std::string &key_name, std::array<uint8_t, BYTES_PER_DATA> &value);
	int clear(const std::string &key_name);
	int read_range(const std::string &begin_key_name, const std::string &end_key_name, std::vector<std::array<uint8_t, BYTES_PER_DATA>> &data);
	int clear_range(const std::string &begin_key_name, const std::string &end_key_name);

private:
	int fetch_branch(blkid_t leaf_id);
	void traverse_branch(blkid_t requested_block_id, enum Operation op, std::array<uint8_t, BYTES_PER_DATA> *value);
	blkid_t find_intersection_bucket(blkid_t leaf_id_1, blkid_t leaf_id_2);
	void swap_blocks(Block &block1, Block &block2);
	int send_branch();
	void store_state();

	int socket_fd;
	struct sockaddr_in server_addr;
	std::map<std::string, blkid_t> key_to_block_id;
	std::unordered_map<blkid_t, blkid_t> position_map;
	std::vector<Block> branch;
	std::unordered_map<blkid_t, std::array<uint8_t, BYTES_PER_DATA>> stash;
	uint8_t enc_key[32];
	uint8_t enc_iv[16];
};

#endif /* PORAM_SINGLE_CLIENT_H */
