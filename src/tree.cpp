/*
	class Tree {
	public:
		Tree();
		Tree(uint16_t num_buckets);

		int set_block(uint16_t block_index);
	private:
		std::vector<Bucket> bucket_array;
	};
 */

#include "tree.h"

Tree::Tree() {}

Tree::Tree(uint16_t num_buckets)
{
	bucket_array.resize(num_buckets);
	for (Bucket &bucket : bucket_array) {
		std::array<Block, BLOCKS_PER_BUCKET> blocks;
		bucket = Bucket(blocks);
	}
}

int Tree::set_block(Block src_block, uint16_t bucket_index, uint16_t block_index)
{
	if (block_index >= BLOCKS_PER_BUCKET)
		return -1;
	return bucket_array[bucket_index].set_indexed_block(src_block, block_index);
}

int Tree::get_path(std::vector<Block> &branch, uint16_t leaf_id)
{
	if (leaf_id == 0)
		return -1;
	leaf_id -= 1;
	uint16_t tree_idx = 1;
	for (uint8_t i = 0; i < 15; ++i) {
		for (Block &block : bucket_array[tree_idx - 1].get_blocks()) {
			branch.push_back(block);
		}
		if (leaf_id & 0x00004000)
			tree_idx = tree_idx * 2 + 1;
		else
			tree_idx = tree_idx * 2;
		leaf_id <<= 1;
	}
	return 0;
}
