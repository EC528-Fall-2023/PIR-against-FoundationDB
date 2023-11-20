#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/time.h>
#include <chrono>
#include "multiclient.h"

int main()
{
	MultiClient client;
	while (1) {
		//struct timeval start_time, end_time;
		std::string option;
		std::string key;
		std::array<uint8_t, BYTES_PER_DATA> data;
		data.fill(0);

		std::cout << "ORAMcli$ ";
		std::cin >> option;

		if (option == "get") {
			std::cin >> key;
			auto start = std::chrono::high_resolution_clock::now();
			//gettimeofday(&start_time, NULL);
			client.get(key, data);
			auto end = std::chrono::high_resolution_clock::now();
			//gettimeofday(&end_time, NULL);

			std::cout << "key: " << key << ", value: " << data.data() << '\n';
			std::chrono::duration<double> elapsed_seconds = end - start;
			//time_t elapsed_seconds = end_time.tv_sec - start_time.tv_sec;
			std::cout << "get: " << elapsed_seconds.count() << " seconds\n";
		} else if (option == "put") {
			std::string value;
			std::cin >> key;
			std::cin.ignore();
			std::getline(std::cin, value);
			memcpy(data.data(), value.data(), value.size());

			auto start = std::chrono::high_resolution_clock::now();
			//gettimeofday(&start_time, NULL);
			client.put(key, data);
			auto end = std::chrono::high_resolution_clock::now();
			//gettimeofday(&end_time, NULL);

			std::cout << "put key: " << key << ", value: " << data.data() << '\n';
			std::chrono::duration<double> elapsed_seconds = end - start;
			//time_t elapsed_seconds = end_time.tv_sec - start_time.tv_sec;
			std::cout << "put: " << elapsed_seconds.count() << " seconds\n";
		} else if (option == "clear") {
			std::cin >> key;

			auto start = std::chrono::high_resolution_clock::now();
			//gettimeofday(&start_time, NULL);
			client.clear(key);
			auto end = std::chrono::high_resolution_clock::now();
			//gettimeofday(&end_time, NULL);

			std::cout << "cleared: " << key << '\n';
			std::chrono::duration<double> elapsed_seconds = end - start;
			//time_t elapsed_seconds = end_time.tv_sec - start_time.tv_sec;
			std::cout << "clear: " << elapsed_seconds.count() << " seconds\n";
		} else if (option == "q" || option == "quit") {
			return 0;
		} else {
			std::cout << "usage: <operation> <key> <data>\n";
		}
/*
		std::cin >> option;
		if (option.size() == 1 && option[0] != 'q') {
			std::cout << "key: ";
			std::cin >> key;
			switch (option[0]) {
			case 'r': {
				gettimeofday(&start_time, NULL);
				client.get(key, data);
				gettimeofday(&end_time, NULL);

				std::cout << "key: " << key << ", value: " << data.data() << '\n';
				time_t elapsed_seconds = end_time.tv_sec - start_time.tv_sec;
				std::cout << "get: " << elapsed_seconds << " seconds\n";
				break;
			} case 'w': {
				std::string value;
				std::cout << "value: ";
				std::cin.ignore();
				std::getline(std::cin, value);
				memcpy(data.data(), value.data(), value.size());

				gettimeofday(&start_time, NULL);
				client.put(key, data);
				gettimeofday(&end_time, NULL);

				time_t elapsed_seconds = end_time.tv_sec - start_time.tv_sec;
				std::cout << "put: " << elapsed_seconds << " seconds\n";
				break;
			} case 'c': {
				gettimeofday(&start_time, NULL);
				client.clear(key);
				gettimeofday(&end_time, NULL);

				std::cout << "cleared: austin" << '\n';
				time_t elapsed_seconds = end_time.tv_sec - start_time.tv_sec;
				std::cout << "clear: " << elapsed_seconds << " seconds\n";
				break;
			} default:
				return 0;
			}
		} else {
			return 0;
		}
*/
	}
	return 0;
}
