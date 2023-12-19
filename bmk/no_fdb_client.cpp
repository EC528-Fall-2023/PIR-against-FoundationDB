#include "block.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include <fstream>
#include <array>
#include <chrono>

#ifdef DEBUG
	#include <error.h>
	#define ERROR(function) error_at_line(0, errno, __FILE__, __LINE__, "%s: %s failed", __func__, function);
#else
	#define ERROR(function)
#endif

#define PORT 8080

#define PUT ((uint8_t) 1)
#define GET ((uint8_t) 2)
#define CLEAR ((uint8_t) 3)

inline int put(std::string key, const std::array<uint8_t, BYTES_PER_DATA> &data);
inline int get(std::string key, std::array<uint8_t, BYTES_PER_DATA> &data);
inline int clear(std::string key);
inline int shutdown_fdb_server();

uint8_t enc_key[32] = {0};
uint8_t enc_iv[16] = {0};
int socket_fd;
struct sockaddr_in server_addr;

int main(int argc, char **argv)
{
	int32_t limit;
	if (argc == 2)
		limit = atoi(argv[1]);
	else
		return -1;

	int encryption_fd;
	if ( (encryption_fd = open(".oram_enc", O_RDONLY)) == -1 ) {
		ERROR("open");
		return -1;
	}

	if (read(encryption_fd, enc_key, 32) != 32 || lseek(encryption_fd, 1, SEEK_CUR) != 33 || read(encryption_fd, enc_iv, 16) != 16) {
		ERROR("read encryption keys");
		return -1;
	}
	close(encryption_fd);

	if ( (socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		ERROR("socket");
		return -1;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);

	if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
		ERROR("inet_pton");
		close(socket_fd);
		return -1;
	}

	if (connect(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0 ) {
		ERROR("connect");
		return -1;
	}

	std::ifstream values("bmk/values.txt");
	std::string value;
	std::string key;
	std::array<uint8_t, BYTES_PER_DATA> data;

	int out_file;
	std::string out_file_path = "bmk_out/putWithoutFDB_" + std::to_string(BLOCK_SIZE) + "_" + std::to_string(BLOCKS_PER_BUCKET) + "_" + std::to_string(TREE_LEVELS) + "_" + std::to_string(limit) + ".txt";
	if ( (out_file = open(out_file_path.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0666)) == -1 ) {
		perror("open");
		return -1;
	}
	printf("starting put\n");
	for (int32_t i = 0; i < limit && std::getline(values, value); ++i) {
	printf("starting put\n");
		system("sync && echo 3 > /proc/sys/vm/drop_caches");

		key = std::to_string(i);
		memset(data.data(), 0, BYTES_PER_DATA);
		memcpy(data.data(), value.data(), value.size());

		auto tic = std::chrono::high_resolution_clock::now();
		put(key, data);
		auto toc = std::chrono::high_resolution_clock::now();

		std::chrono::duration<double> elapsed = toc - tic;

		dprintf(out_file, "%f\n", elapsed.count());
		if (i % 10 == 0)
			printf("%i\n", i);
	}

	close(out_file);
	values.seekg(0, std::ios::beg);
	
	out_file_path = "bmk_out/getWithoutFDB_" + std::to_string(BLOCK_SIZE) + "_" + std::to_string(BLOCKS_PER_BUCKET) + "_" + std::to_string(TREE_LEVELS) + "_" + std::to_string(limit) + ".txt";
	if ( (out_file = open(out_file_path.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0666)) == -1 ) {
		perror("open");
		return -1;
	}
	printf("starting get\n");
	for (int32_t i = 0; i < limit && std::getline(values, value); ++i) {
		system("sync && echo 3 > /proc/sys/vm/drop_caches");

//		std::array<uint8_t, BYTES_PER_DATA> data2;
		key = std::to_string(i);
		memset(data.data(), 0, BYTES_PER_DATA);
//		memset(data2.data(), 0, BYTES_PER_DATA);
//		memcpy(data2.data(), value.data(), value.size());

		auto tic = std::chrono::high_resolution_clock::now();
		get(key, data);
		auto toc = std::chrono::high_resolution_clock::now();

//		if (memcmp(data.data(), data2.data(), BYTES_PER_DATA) != 0) {
//			printf("DATA IS NOT THE SAME\n");
//		}

		std::chrono::duration<double> elapsed = toc - tic;

		dprintf(out_file, "%f\n", elapsed.count());
		if (i % 10 == 0)
			printf("%i\n", i);
	}

	close(out_file);
	values.seekg(0, std::ios::beg);
	
	out_file_path = "bmk_out/clear_" + std::to_string(BLOCK_SIZE) + "_" + std::to_string(BLOCKS_PER_BUCKET) + "_" + std::to_string(TREE_LEVELS) + "_" + std::to_string(limit) + ".txt";
	if ( (out_file = open(out_file_path.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0666)) == -1 ) {
		perror("open");
		return -1;
	}
	printf("starting clear\n");
	for (int32_t i = 0; i < limit; ++i) {
		system("sync && echo 3 > /proc/sys/vm/drop_caches");

		key = std::to_string(i);

		auto tic = std::chrono::high_resolution_clock::now();
		clear(key);
		auto toc = std::chrono::high_resolution_clock::now();

		std::chrono::duration<double> elapsed = toc - tic;

		dprintf(out_file, "%f\n", elapsed.count());
		if (i % 10 == 0)
			printf("%i\n", i);
	}

	printf("end");

	shutdown_fdb_server();

	return 0;

	return 0;
}

inline int put(std::string key, const std::array<uint8_t, BYTES_PER_DATA> &data)
{
	// send operation type
	uint8_t op = PUT;
	if (send(socket_fd, &op, sizeof(op), 0) != sizeof(op)) {
		ERROR("send");
		return -1;
	}

	// send key to server
	uint32_t key_size = key.size();
	if (send(socket_fd, &key_size, sizeof(key_size), 0) != sizeof(key_size)) {
		ERROR("send");
		return -1;
	}
	if (send(socket_fd, key.c_str(), key_size, 0) != key_size) {
		ERROR("send");
		return -1;
	}
	
	// send data to server
	if (send(socket_fd, data.data(), BYTES_PER_DATA, 0) != BYTES_PER_DATA) {
		ERROR("send");
		return -1;
	}

	// confirm put success
	uint8_t confirmation = 0;
	if (recv(socket_fd, &confirmation, sizeof(confirmation), 0) != sizeof(confirmation)) {
		ERROR("recv");
		return -1;
	}
	if (confirmation != 1) {
		ERROR("confirmation");
		return -1;
	}

	return 0;
}

inline int get(std::string key, std::array<uint8_t, BYTES_PER_DATA> &data)
{
	// send operation type
	uint8_t op = GET;
	if (send(socket_fd, &op, sizeof(op), 0) != sizeof(op)) {
		ERROR("send");
		return -1;
	}

	// send key to server
	uint32_t key_size = key.size();
	if (send(socket_fd, &key_size, sizeof(key_size), 0) != sizeof(key_size)) {
		ERROR("send");
		return -1;
	}
	if (send(socket_fd, key.c_str(), key_size, 0) != key_size) {
		ERROR("send");
		return -1;
	}
	
	// recv data from server
	if (recv(socket_fd, data.data(), BYTES_PER_DATA, MSG_WAITALL) != BYTES_PER_DATA) {
		ERROR("recv");
		return -1;
	}

	return 0;
}

inline int clear(std::string key)
{
	// send operation type
	uint8_t op = CLEAR;
	if (send(socket_fd, &op, sizeof(op), 0) != sizeof(op)) {
		ERROR("send");
		return -1;
	}

	// send key to server
	uint32_t key_size = key.size();
	if (send(socket_fd, &key_size, sizeof(key_size), 0) != sizeof(key_size)) {
		ERROR("send");
		return -1;
	}
	if (send(socket_fd, key.c_str(), key_size, 0) != key_size) {
		ERROR("send");
		return -1;
	}

	// confirm put success
	uint8_t confirmation = 0;
	if (recv(socket_fd, &confirmation, sizeof(confirmation), 0) != sizeof(confirmation)) {
		ERROR("recv");
		return -1;
	}
	if (confirmation != 1) {
		ERROR("confirmation");
		return -1;
	}

	return 0;
}

inline int shutdown_fdb_server()
{
	if (socket_fd == -1) {
		ERROR("socket_fd");
		return -1;
	}

	// request_id = 0 -> exit server process
	blkid_t request_id = 0;
	if (send(socket_fd, &request_id, sizeof(request_id), 0) != sizeof(request_id)) {
		ERROR("send");
		close(socket_fd);
		return -1;
	}

	return 0;
}
