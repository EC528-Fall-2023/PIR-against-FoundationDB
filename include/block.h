#ifndef PORAM_BLOCK_H
#define PORAM_BLOCK_H

#include <cstdint>
#include <array>
#include <random>

#define BYTES_PER_BLOCK 1024
#define BLOCKS_PER_BUCKET 3

class Block {
public:
	Block();
	Block(const Block &block);
	Block(uint16_t leaf_id, const std::array<uint8_t, BYTES_PER_BLOCK>& data);

	uint16_t get_leaf_id();
	std::array<uint8_t, BYTES_PER_BLOCK> &get_data();

	void set_leaf_id(uint16_t leaf_id);
	void set_data(const std::array<uint8_t, BYTES_PER_BLOCK>& data);
	void set_random_data();

private:
	uint16_t leaf_id;
	std::array<uint8_t, BYTES_PER_BLOCK> data;
};

#endif /* PORAM_BLOCK_H */
