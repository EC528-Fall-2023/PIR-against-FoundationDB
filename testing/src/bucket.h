#ifndef PORAM_BUCKET_H
#define PORAM_BUCKET_H

#include "block.h"

#define BLOCKS_PER_BUCKET 3

class Bucket {
public:
	Bucket();
	Bucket(const std::vector<Block>& blocks);

	std::vector<Block> get_blocks();
	void set_blocks(const std::vector<Block>& blocks);
private:
	std::vector<Block> blocks;
};

/*
BRANCH METADATA:
uint32_t num_buckets	// can infer num_blocks = num_buckets * BLOCKS_PER_BUCKET
uint32_t block_size[0]
uint32_t block_size[1]
uint32_t block_size[2]
etc...
block_size[0] block_data[0]
block_size[1] block_data[1]
block_size[2] block_data[2]
etc...
*/

#endif /* PORAM_BUCKET_H */
