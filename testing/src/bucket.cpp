/*
class Bucket {
public:
	Bucket();
	Bucket(const std::vector<Block>& blocks);

	std::vector<Block> get_blocks();
	void set_blocks(const std::vector<Block>& blocks);
private:
	std::vector<Block> blocks;
};
*/

#include "bucket.h"

Bucket::Bucket()
{
	for (int i = 0; i < BLOCKS_PER_BUCKET; ++i) {
		blocks.push_back(Block(0, i, std::vector<uint8_t> ()));
	}
}

Bucket::Bucket(const std::vector<Block>& blocks)
{
	this->blocks = blocks;
}

std::vector<Block> Bucket::get_blocks()
{
	return blocks;
}

void Bucket::set_blocks(const std::vector<Block>& blocks)
{
	this->blocks = blocks;
}

