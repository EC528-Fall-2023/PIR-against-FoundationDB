#include <single_client.h>

int socket_fd;
struct sockaddr_in server_addr;

std::vector<uint32_t> generate_values(uint32_t n, uint32_t m) {
	if (n > m) {
		std::cerr << "Error: Cannot generate more unique values than total numbers available.\n";
		return {};
	}

	std::vector<uint32_t> numbers(m, 0);
	for (uint32_t i = 1; i <= n; ++i) {
		numbers[i] = i;
	}

	std::mt19937 gen(42);

	// Shuffle the vector
	std::shuffle(numbers.begin(), numbers.end(), gen);

	return numbers;
}

int main()
{
	if ( (socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		ERROR("socket");
		return -1;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);

	if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
		ERROR("inet_pton");
		close(socket_fd);
		return -1;
	}

	if (connect(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0 ) {
		ERROR("connect");
		return -1;
	}

	uint8_t fdb_is_initialized;
	if (recv(socket_fd, &fdb_is_initialized, sizeof(fdb_is_initialized), 0) != sizeof(fdb_is_initialized)) {
		ERROR("send");
		return -1;
	}

	if (fdb_is_initialized) {
		ERROR("fdb is already initialized");
		return 0;
	}

	random_block_ids = generate_values(BLOCKS_PER_BUCKET * BUCKETS_PER_TREE / 2, BLOCKS_PER_BUCKET * BUCKETS_PER_TREE);

	uint8_t data[BYTES_PER_DATA];
	for (uint32_t i = 0; i < BLOCKS_PER_BUCKET * BUCKETS_PER_TREE - 1; ++i) {
		memset(data, 0, BYTES_PER_DATA);
		for (uint32_t j = 0; j < BYTES_PER_DATA; ++j) {
			data[j] = RAND();
		}
		if (random_block_ids[i] == 0) {
			Block block(0, data, BYTES_PER_DATA);
		else
			Block block(random_block_ids[i], data, BYTES_PER_DATA);
		if (block.encrypt(enc_key, enc_iv) == -1) {
			ERROR("encrypt");
			return -1;
		}
		if (send(socket_fd, block.get_encrypted_data(), BLOCK_SIZE, MSG_MORE) != BLOCK_SIZE) {
			ERROR("send");
			return -1;
		}
	}
	memset(data, 0, BYTES_PER_DATA);
	for (uint32_t j = 0; j < BYTES_PER_DATA; ++j) {
		data[j] = RAND();
	}
	Block block(0, data, BYTES_PER_DATA);
	if (block.encrypt(enc_key, enc_iv) == -1) {
		ERROR("encrypt");
		return -1;
	}
	if (send(socket_fd, block.get_encrypted_data(), BLOCK_SIZE, 0) != BLOCK_SIZE) {
		ERROR("send");
		return -1;
	}
	if (recv(socket_fd, &fdb_is_initialized, sizeof(fdb_is_initialized), 0) != sizeof(fdb_is_initialized)) {
		ERROR("recv");
		return -1;
	}
	if (!fdb_is_initialized) {
		ERROR("fdb is not initialized");
		return -1;
	}

	return 0;
}
