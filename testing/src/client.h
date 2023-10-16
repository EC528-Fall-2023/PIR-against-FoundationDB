#ifndef PORAM_CLIENT_H
#define PORAM_CLIENT_H

#include <unistd.h>
#include <iostream>
#include <cstring>
#include <string>
#include <map>
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

	int put(const std::string &key_name, const std::string &value);
	int get(const std::string &key_name, std::string &value);
	int read_range(const std::string &begin_key_name, const std::string &end_key_name);
	int clear_range(const std::string &begin_key_name, const std::string &end_key_name);

private:
/*
	PathOramClient(const std::string &server_ip = "127.0.0.1", const int port = 8080);
	static PathOramClient *instance;
*/
	int send_and_receive_request(uint32_t leaf_id, std::string &data_buffer);

	int socket_fd;
	std::map<std::string, uint32_t> position_map;
};

enum Operation {
	READ = 1,
	WRITE,
	READ_RANGE,
	CLEAR_RANGE
};

#endif
