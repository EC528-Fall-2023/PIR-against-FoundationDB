#ifndef PORAM_BLOCK_H
#define PORAM_BLOCK_H

#include <stdint.h>
#include <array>

#define BYTES_PER_BLOCK 1024

class Block {
public:
	Block();
	Block(uint16_t leaf_id, const std::array<uint8_t, BYTES_PER_BLOCK>& data);

	uint16_t get_leaf_id();
	std::array<uint8_t, BYTES_PER_BLOCK> get_data();

	void set_leaf_id(uint16_t leaf_id);
	void set_data(const std::array<uint8_t, BYTES_PER_BLOCK>& data);

private:
	uint16_t leaf_id;
	std::array<uint8_t, BYTES_PER_BLOCK> data;
};

#endif /* PORAM_BLOCK_H */
