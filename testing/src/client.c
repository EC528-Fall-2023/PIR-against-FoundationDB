#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <array>

#define PORT 8080

int main(int argc, char **argv)
{
	int socket_fd;
	if ( (socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		printf("socket: failed\n");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);

	if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
		printf("inet_pton: failed\n");
		exit(EXIT_FAILURE);
	}

	int status;
	if ( (status = connect(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr))) < 0 ) {
		printf("connect: failed\n");
		exit(EXIT_FAILURE);
	}

	std::array<char, 8> data = {'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o'};

	send(socket_fd, data.data(), 8, 0);
	//send(socket_fd, "hello", sizeof("hello"), 0);
	printf("Client: Hello message sent\n");

	return 0;
}
