#include <stdio.h>
#include <unistd.h>
#include "../include/client.h"

int main()
{
	put((unsigned char *)"placeholder text", 16, (unsigned char *)"hello user", 10);
	sleep(3);
	put((unsigned char *)"placeholder text", 16, (unsigned char *)"goodbye user", 12);
	return 0;
}
