#ifndef PORAM_BLOCK_H
#define PORAM_BLOCK_H

#include <stdint.h>
#include <vector>

class Block {
public:
	Block();
	Block(uint32_t leaf_id, uint32_t bucket_idx, const std::vector<uint8_t>& data);

	uint32_t get_leaf_id();
	uint32_t get_bucket_idx();
	std::vector<uint8_t> get_data();

	void set_leaf_id(uint32_t leaf_id);
	void set_bucket_idx(uint32_t bucket_idx);
	void set_data(const std::vector<uint8_t>& data);

private:
	uint32_t leaf_id;
	uint32_t bucket_idx;
	std::vector<uint8_t> data;
};

#endif /* PORAM_BLOCK_H */
