#include <iostream>
#include <unistd.h>
#include "../include/client.h"

int main()
{
	PathOramClient client;
	std::vector<uint8_t> data (1, 8);
	client.get("austin", data);
	sleep(3);
	client.get("austin", data);
	return 0;
}
