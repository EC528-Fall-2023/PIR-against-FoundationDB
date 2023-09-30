#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080

int main(int argc, char **argv)
{
	int server_socket;
	if ( (server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		printf("socket: failed\n");
		exit(EXIT_FAILURE);
	}

	int opt = 1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		printf("setsockopt: failed\n");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	if (bind(server_socket, (struct sockaddr *) &address, sizeof(address)) < 0) {
		printf("bind: failed\n");
		exit(EXIT_FAILURE);
	}

	if (listen(server_socket, 3) < 0) {
		printf("listen: failed\n");
		exit(EXIT_FAILURE);
	}

	int client_socket;
	socklen_t addr_len = sizeof(address);
	if ( (client_socket = accept(server_socket, (struct sockaddr *) &address, &addr_len)) < 0 ) {
		printf("accept: failed\n");
		exit(EXIT_FAILURE);
	}

	char buffer[1024] = {0};
	read(client_socket, buffer, 1024);
	printf("Server: message received: %s\n", buffer);

	close(client_socket);
	shutdown(server_socket, SHUT_RDWR);

	return 0;
}
