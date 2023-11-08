#include "multiclient.h"

static std::random_device generator;
static std::uniform_int_distribution<uint16_t> random_request_id(0, 0xffff);

MultiClient::MultiClient(const std::string &server_ip, const int port)
{
	if ( (socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		printf("socket: failed\n");
		exit(EXIT_FAILURE);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
		printf("inet_pton: failed\n");
		//close(socket_fd);
		exit(EXIT_FAILURE);
	}

#ifdef DEBUG
	std::cout << "client: starting multiclient\n";
#endif
}

MultiClient::~MultiClient()
{
}

int MultiClient::put(const std::string &key_name, const std::array<uint8_t, BYTES_PER_BLOCK> &value)
{
	uint16_t request_id = random_request_id(generator);
	if (send_request(request_id, WRITE) != 0) {
		std::cerr << "client: send_request failed\n";
		return -1;
	}

	if (send_key(key_name) != 0) {
		std::cerr << "client: send_key failed\n";
		return -1;
	}

	if (send(socket_fd, value.data(), BYTES_PER_BLOCK, 0) != BYTES_PER_BLOCK) {
		std::cerr << "client: send failed\n";
		return -1;
	}

	if (confirm_request(request_id) != 0) {
		std::cerr << "client: confirm_request failed\n";
		return -1;
	}

	return 0;
}

int MultiClient::get(const std::string &key_name, std::array<uint8_t, BYTES_PER_BLOCK> &value)
{
	uint16_t request_id = random_request_id(generator);
	if (send_request(request_id, READ) != 0) {
		std::cerr << "client: send_request failed\n";
		return -1;
	}

	if (send_key(key_name) != 0) {
		std::cerr << "client: send_key failed\n";
		return -1;
	}

	if (recv(socket_fd, value.data(), BYTES_PER_BLOCK, 0) != BYTES_PER_BLOCK) {
		std::cerr << "client: send failed\n";
		return -1;
	}

	if (confirm_request(request_id) != 0) {
		std::cerr << "client: confirm_request failed\n";
		return -1;
	}

	return 0;
}

int MultiClient::clear(const std::string &key_name)
{
	uint16_t request_id = random_request_id(generator);
	if (send_request(request_id, CLEAR) != 0) {
		std::cerr << "client: send_request failed\n";
		return -1;
	}

	if (send_key(key_name) != 0) {
		std::cerr << "client: send_key failed\n";
		return -1;
	}

	if (confirm_request(request_id) != 0) {
		std::cerr << "client: confirm_request failed\n";
		return -1;
	}

	return 0;
}

int MultiClient::read_range(const std::string &begin_key_name, const std::string &end_key_name, std::vector<std::array<uint8_t, BYTES_PER_BLOCK>> &data)
{
	uint16_t request_id = random_request_id(generator);
	if (send_request(request_id, READ_RANGE) != 0) {
		std::cerr << "client: send_request failed\n";
		return -1;
	}

	if (send_key(begin_key_name) != 0 || send_key(end_key_name) != 0) {
		std::cerr << "client: send_key failed\n";
		return -1;
	}

	uint32_t num_data;
	if (recv(socket_fd, &num_data, sizeof(num_data), 0) != sizeof(num_data)) {
		std::cerr << "client: recv failed\n";
		return -1;
	}
	data.resize(num_data);
	for (uint32_t current_data_idx = 0; current_data_idx < num_data; ++current_data_idx) {
		if (recv(socket_fd, data[current_data_idx].data(), BYTES_PER_BLOCK, 0) != BYTES_PER_BLOCK) {
			std::cerr << "client: recv failed: " << current_data_idx << '\n';
			return -1;
		}
	}

	if (confirm_request(request_id) != 0) {
		std::cerr << "client: confirm_request failed\n";
		return -1;
	}

	return 0;
}

int MultiClient::clear_range(const std::string &begin_key_name, const std::string &end_key_name)
{
	uint16_t request_id = random_request_id(generator);
	if (send_request(request_id, CLEAR) != 0) {
		std::cerr << "client: send_request failed\n";
		return -1;
	}

	if (send_key(begin_key_name) != 0 || send_key(end_key_name) != 0) {
		std::cerr << "client: send_key failed\n";
		return -1;
	}

	if (confirm_request(request_id) != 0) {
		std::cerr << "client: confirm_request failed\n";
		return -1;
	}

	return 0;
}

int MultiClient::send_request(const uint16_t request_id, const uint8_t operation)
{
	if (connect(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0 ) {
		std::cerr << "client: connect: failed\n";
		return -1;
	}
#ifdef DEBUG
	std::cout << "client: connection to " << server_addr.sin_addr.s_addr << " established\n";
#endif

	if (send(socket_fd, &request_id, sizeof(request_id), 0) != sizeof(request_id)) {
		std::cerr << "client: send failed\n";
		return -1;
	}

	if (send(socket_fd, &operation, sizeof(operation), 0) != sizeof(operation)) {
		std::cout << "client: send failed\n";
		return -1;
	}

	return 0;
}

int MultiClient::send_key(const std::string &key)
{
	uint32_t key_size = key.size();
	if (send(socket_fd, &key_size, sizeof(key_size), 0) != sizeof(key_size)) {
		std::cerr << "client: send failed\n";
		return -1;
	}

	if (send(socket_fd, key.data(), key_size, 0) != key_size) {
		std::cerr << "client: send failed\n";
		return -1;
	}
#ifdef DEBUG
	std::cout << "client: sent " << key << " to master client\n";
#endif

	return 0;
}

int MultiClient::confirm_request(const uint16_t request_id)
{
	uint16_t confirmation_id;
	if (recv(socket_fd, &confirmation_id, sizeof(confirmation_id), 0) != sizeof(confirmation_id)) {
		std::cerr << "client: recv failed\n";
		return -1;
	}

	if (request_id != confirmation_id) {
		std::cerr << "client: incorrect confirmation id\n";
		return -1;
	}

	close(socket_fd);
#ifdef DEBUG
	std::cout << "client: GET SUCCESS\n";
	std::cout << "client: disconnected\n";
#endif

	return 0;
}
