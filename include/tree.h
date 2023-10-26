#ifndef PORAM_TREE_H
#define PORAM_TREE_H

#include "bucket.h"
#include "block.h"
#include <vector>

class Tree {
public:
	Tree();
	Tree(uint16_t num_buckets);

	int set_block(Block src_block, uint16_t bucket_index, uint16_t block_index);
	Block get_block(uint16_t bucket_index, uint16_t block_index);
	int get_path(std::vector<Block> &branch, uint16_t leaf_id);
private:
	std::vector<Bucket> bucket_array;
};

#endif /* PORAM_TREE_H */
