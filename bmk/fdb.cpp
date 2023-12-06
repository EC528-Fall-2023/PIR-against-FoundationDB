#include "single_client.h"
#include <string>
#include <fstream>
#include <chrono>

int main(int argc, char** argv)
{
	int32_t limit;
	if (argc == 2)
		limit = atoi(argv[1]);
	else
		return -1;

	SingleClient client;
	client.initialize();

	std::ifstream values("bmk/values.txt");
	std::string value;
	std::string key;
	std::array<uint8_t, BYTES_PER_DATA> data;

	int out_file;
	std::string out_file_path = "bmk_out/put_" + std::to_string(BLOCK_SIZE) + "_" + std::to_string(BLOCKS_PER_BUCKET) + "_" + std::to_string(TREE_LEVELS) + "_" + std::to_string(limit) + ".txt";
	if ( (out_file = open(out_file_path.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0666)) == -1 ) {
		perror("open");
		return -1;
	}
	printf("starting put\n");
	for (int32_t i = 0; i < limit && std::getline(values, value); ++i) {
		system("sync && echo 3 > /proc/sys/vm/drop_caches");

		key = std::to_string(i);
		memset(data.data(), 0, BYTES_PER_DATA);
		memcpy(data.data(), value.data(), value.size());

		auto tic = std::chrono::high_resolution_clock::now();
		client.put(key, data);
		auto toc = std::chrono::high_resolution_clock::now();

		std::chrono::duration<double> elapsed = toc - tic;

		dprintf(out_file, "%f\n", elapsed.count());
		if (i % 10 == 0)
			printf("%i\n", i);
	}

	close(out_file);
	values.seekg(0, std::ios::beg);
	
	out_file_path = "bmk_out/get_" + std::to_string(BLOCK_SIZE) + "_" + std::to_string(BLOCKS_PER_BUCKET) + "_" + std::to_string(TREE_LEVELS) + "_" + std::to_string(limit) + ".txt";
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
		client.get(key, data);
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
		client.clear(key);
		auto toc = std::chrono::high_resolution_clock::now();

		std::chrono::duration<double> elapsed = toc - tic;

		dprintf(out_file, "%f\n", elapsed.count());
		if (i % 10 == 0)
			printf("%i\n", i);
	}

	printf("end");

	client.shutdown();

	return 0;
}
