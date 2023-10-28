#include <iostream>
#include <unistd.h>
#include "client.h"

int main()
{
	PathOramClient client;
	std::array<uint8_t, BYTES_PER_BLOCK> data;
	data[0] = 8;
	client.get("austin", data);
	client.get("austin", data);
	return 0;
}
