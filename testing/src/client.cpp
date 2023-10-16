/*
class PathOramClient {
public:
	PathOramClient(const std::string &server_ip);
	~PathOramClient();

	int put(const std::string &key_name, const std::string &value);
	int get(const std::string &key_name, std::string &value);
	int read_range(const std::string &begin_key_name, const std::string &end_key_name);
	int clear_range(const std::string &begin_key_name, const std::string &end_key_name);

private:
	
};
*/

#include "client.h"

PathOramClient::PathOramClient(const std::string &server_ip, const int port)
{
	if ( (socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		printf("socket: failed\n");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
		printf("inet_pton: failed\n");
		//close(socket_fd);
		exit(EXIT_FAILURE);
	}

	int status;
	if ( (status = connect(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr))) < 0 ) {
		printf("connect: failed\n");
		//close(socket_fd);
		exit(EXIT_FAILURE);
	}
}

PathOramClient::~PathOramClient()
{

}

int PathOramClient::put(const std::string &key_name, const std::string &value)
{
	return 0;
}

// TODO: implement first
// TODO: replace all string data values to vector
int PathOramClient::get(const std::string &key_name, std::string &value)
{
	send_request(position_map[key_name]);
	return 0;
}

int PathOramClient::read_range(const std::string &begin_key_name, const std::string &end_key_name)
{
	return 0;
}

int PathOramClient::clear_range(const std::string &begin_key_name, const std::string &end_key_name)
{
	return 0;
}

int PathOramClient::send_request(uint32_t leaf_id, std::string &data_buffer)
{
	// request => leaf_id (4 bytes)
	if (send(socket_fd, leaf_id, 4, 0) != 4) {
		std::cerr << "send: failed" << std::endl;
		exit(EXIT_FAILURE);
	}

	uint32_t data_size;
	if (recv(socket_fd, &data_size, 4, 0) != 4) {
		std::cerr << "recv: failed" << std::endl;
		exit(EXIT_FAILURE);
	}

	uint8_t *cdata_buffer = (uint8_t *) malloc(data_size);
	if (recv(socket_fd, cdata_buffer, data_size, 0) != data_size) {
		std::cerr << "recv: failed" << std::endl;
		exit(EXIT_FAILURE);
	}
	data_buffer.assign(cdata_buffer);
}
