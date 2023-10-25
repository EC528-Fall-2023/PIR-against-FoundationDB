/*
class Bucket {
public:
	Bucket();
	Bucket(const std::array<Block, BLOCKS_PER_BUCKET>& blocks);

	std::array<Block, BLOCKS_PER_BUCKET> get_blocks();
	void set_blocks(const std::array<Block, BLOCKS_PER_BUCKET>& blocks);
private:
	std::array<Block, BLOCKS_PER_BUCKET> blocks;
};
*/

#include "bucket.h"

Bucket::Bucket()
{
	std::array<uint8_t, BYTES_PER_BLOCK> temp;
	for (int i = 0; i < BLOCKS_PER_BUCKET; ++i) {
		blocks[i] = Block(0, temp);
	}
}

Bucket::Bucket(const std::array<Block, BLOCKS_PER_BUCKET>& blocks)
{
	this->blocks = blocks;
}

std::array<Block, BLOCKS_PER_BUCKET> Bucket::get_blocks()
{
	return blocks;
}

void Bucket::set_blocks(const std::array<Block, BLOCKS_PER_BUCKET>& blocks)
{
	this->blocks = blocks;
}

int Bucket::set_indexed_block(Block block, uint32_t index)
{
	if (index >= BLOCKS_PER_BUCKET)
		return -1;
	blocks[index] = block;
	return 0;
}
