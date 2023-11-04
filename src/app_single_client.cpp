#include <iostream>
#include <unistd.h>
#include "single_client.h"

int main()
{
	SingleClient client;

	while (1) {
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
			case 'r':
				client.get(key, data);
				std::cout << "key: austin, value: " << data.data() << '\n';
				break;
			case 'w':
				client.put(key, data);
				break;
			default:
				return 0;
			}
		} else {
			std::cout << "error\n";
			continue;
		}
	}
	return 0;
}
