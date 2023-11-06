#include <iostream>
#include <unistd.h>
#include <sys/time.h>
#include "single_client.h"

int main()
{
	SingleClient client;

	while (1) {
		struct timeval start_time, end_time;
		std::string key;
		std::string option;
		std::array<uint8_t, BYTES_PER_BLOCK> data;
		data.fill(0);
		std::cout << "read (r) or write (w)?\n";
		std::cin >> option;
		if (option.size() == 1) {
			std::cout << "key: ";
			std::cin >> key;
			switch (option[0]) {
			case 'r': {
				gettimeofday(&start_time, NULL);
				client.get(key, data);
				gettimeofday(&end_time, NULL);
				std::cout << "key: austin, value: " << data.data() << '\n';
				time_t elapsed_seconds = end_time.tv_sec - start_time.tv_sec;
				suseconds_t elapsed_microseconds = end_time.tv_usec - start_time.tv_usec;
				std::cout << "get: " << elapsed_seconds << '.' << elapsed_microseconds << " seconds\n";
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
				suseconds_t elapsed_microseconds = end_time.tv_usec - start_time.tv_usec;
				std::cout << "put: " << elapsed_seconds << '.' << elapsed_microseconds << " seconds\n";
				break;
			} default:
				return 0;
			}
		} else {
			std::cout << "error\n";
			continue;
		}
	}
	return 0;
}
