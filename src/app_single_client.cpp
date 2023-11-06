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
		std::cout << "read (r), write (w), clear (c), quit(q)?\n";
		std::cin >> option;
		if (option.size() == 1 && option[0] != 'q') {
			std::cout << "key: ";
			std::cin >> key;
			switch (option[0]) {
			case 'r': {
				gettimeofday(&start_time, NULL);
				client.get(key, data);
				gettimeofday(&end_time, NULL);

				std::cout << "key: austin, value: " << data.data() << '\n';
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
	}
	return 0;
}
