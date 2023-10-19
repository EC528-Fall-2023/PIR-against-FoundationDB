#ifndef PORAM_BLOCK_H
#define PORAM_BLOCK_H

#include <stdint.h>
#include <array>

#define BYTES_PER_BLOCK 65536

class Block {
public:
	Block();
	Block(uint32_t leaf_id, uint32_t bucket_idx, const std::array<uint8_t, BYTES_PER_BLOCK>& data);

	uint32_t get_leaf_id();
	uint32_t get_bucket_idx();
	std::array<uint8_t, BYTES_PER_BLOCK> get_data();

	void set_leaf_id(uint32_t leaf_id);
	void set_bucket_idx(uint32_t bucket_idx);
	void set_data(const std::array<uint8_t, BYTES_PER_BLOCK>& data);

private:
	uint32_t leaf_id;
	uint32_t bucket_idx;
	std::array<uint8_t, BYTES_PER_BLOCK> data;
};

#endif /* PORAM_BLOCK_H */
