#ifndef PORAM_MULTICLIENT_H
#define PORAM_MULTICLIENT_H

#include "block.h"
#include <iostream>
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <random>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

enum Operation {
	READ,
	WRITE,
	CLEAR,
	READ_RANGE,
	CLEAR_RANGE
};

class MultiClient {
public:
/*
	static MultiClient *get_instance();
	MultiClient() = delete;
	MultiClient(const MultiClient&) = delete;
	MultiClient& operator=(const MultiClient&) = delete;
*/
	MultiClient(const std::string &server_ip = "127.0.0.1", const int port = 8081);
	~MultiClient();

	int put(const std::string &key_name, const std::array<uint8_t, BYTES_PER_BLOCK> &value);
	int get(const std::string &key_name, std::array<uint8_t, BYTES_PER_BLOCK> &value);
	int clear(const std::string &key_name);
	int read_range(const std::string &begin_key_name, const std::string &end_key_name, std::vector<std::array<uint8_t, BYTES_PER_BLOCK>> &data);
	int clear_range(const std::string &begin_key_name, const std::string &end_key_name);

private:
/*
	MultiClient(const std::string &server_ip = "127.0.0.1", const int port = 8080);
	static MultiClient *instance;
*/
	int send_request(const uint16_t request_id, const uint8_t operation);
	int send_key(const std::string &key);
	int confirm_request(const uint16_t request_id);

	int socket_fd;
	struct sockaddr_in server_addr;
};

#endif /* PORAM_MULTICLIENT_H */
