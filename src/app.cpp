#include <iostream>
#include <unistd.h>
#include "client.h"

int main()
{
	PathOramClient client;
	std::array<uint8_t, BYTES_PER_BLOCK> data;
	data[0] = 8;
	client.get("austin", data);
	std::cout << "key: austin, value: " << data.data() << '\n';
	//client.get("austin", data);
	return 0;
}
