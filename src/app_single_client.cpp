#include <iostream>
#include <cstring>
#include <unistd.h>
#include "single_client.h"

int main()
{
	SingleClient client;
	client.initialize();
	while (1) {
		std::string option;
		std::string key;
		std::array<uint8_t, BYTES_PER_DATA> data;
		data.fill(0);

		std::cout << "ORAMcli$ ";
		std::cin >> option;

		if (option == "get") {
			std::cin >> key;
			client.get(key, data);

			std::cout << "key: " << key << ", value: " << data.data() << '\n';
		} else if (option == "put") {
			std::string value;
			std::cin >> key;
			std::cin.ignore();
			std::getline(std::cin, value);
			memcpy(data.data(), value.data(), value.size());

			client.put(key, data);

			std::cout << "put key: " << key << ", value: " << data.data() << '\n';
		} else if (option == "clear") {
			std::cin >> key;

			client.clear(key);

			std::cout << "cleared: austin" << '\n';
		} else if (option == "q" || option == "quit") {
			client.shutdown();
			return 0;
		} else {
			std::cout << "usage: <operation> <key> <data>\n";
		}
	}

	return 0;
}
