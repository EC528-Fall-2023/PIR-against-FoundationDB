#include "single_client.h"
#include <iostream>
#include <chrono>
#include <fstream>

int main()
{
	SingleClient client;
	
	auto tic = std::chrono::high_resolution_clock::now();
	client.initialize();
	auto toc = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> elapsed = toc - tic;

	std::ofstream startup_times("../benchmarking/time_startup.txt", std::ios::app);
	startup_times << elapsed.count() << '\n';
	startup_times.close();

	std::ifstream keys("../benchmarking/keys.txt");
	std::ifstream values("../benchmarking/values.txt");
	std::string key;
	std::string value;
	std::array<uint8_t, BYTES_PER_DATA> data;
	memset(data.data(), 0, BYTES_PER_DATA);

	std::ofstream put_times;
	std::ofstream get_times;
	while (std::getline(keys, key) && std::getline(values, value)) {
		put_times.open("../benchmarking/time_put.txt", std::ofstream::out | std::ofstream::app);
		get_times.open("../benchmarking/time_get.txt", std::ofstream::out | std::ofstream::app);
		memcpy(data.data(), value.data(), value.size());

		tic = std::chrono::high_resolution_clock::now();
		client.put(key, data);
		toc = std::chrono::high_resolution_clock::now();
		elapsed = toc - tic;
		put_times << elapsed.count() << '\n';
		std::cout << "put " << key << '\n';

		memset(data.data(), 0, BYTES_PER_DATA);

		tic = std::chrono::high_resolution_clock::now();
		client.get(key, data);
		toc = std::chrono::high_resolution_clock::now();
		elapsed = toc - tic;
		get_times << elapsed.count() << '\n';
		std::cout << "get " << key << '\n';

		if (memcmp(key.data(), data.data(), key.size()) != 0) {
			std::cout << "DATA NOT THE SAME" << '\n';
			return -1;
		}

		memset(data.data(), 0, BYTES_PER_DATA);
		key.clear();

		put_times.close();
		get_times.close();
	}
	keys.close();
	values.close();

	return 0;
}

