#ifndef PORAM_BUCKET_H
#define PORAM_BUCKET_H

#include "block.h"

#define BLOCKS_PER_BUCKET 3

class Bucket {
public:
	Bucket();
	Bucket(const std::array<Block, BLOCKS_PER_BUCKET>& blocks);

	std::array<Block, BLOCKS_PER_BUCKET> get_blocks();
	void set_blocks(const std::array<Block, BLOCKS_PER_BUCKET>& blocks);
	int set_indexed_block(Block block, uint16_t index);
private:
	std::array<Block, BLOCKS_PER_BUCKET> blocks;
};

#endif /* PORAM_BUCKET_H */
