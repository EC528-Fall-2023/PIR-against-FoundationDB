#include "block.h"

Block::Block()
{
	leaf_id = 0;
	bucket_idx = 0;
}

Block::Block(uint32_t leaf_id, uint32_t bucket_idx, const std::array<uint8_t, BYTES_PER_BLOCK>& data)
{
	this->leaf_id = leaf_id;
	this->bucket_idx = bucket_idx;
	this->data = data;
}

uint32_t Block::get_leaf_id()
{
	return leaf_id;
}

uint32_t Block::get_bucket_idx()
{
	return bucket_idx;
}

std::array<uint8_t, BYTES_PER_BLOCK> Block::get_data()
{
	return data;
}

void Block::set_leaf_id(uint32_t leaf_id)
{
	this->leaf_id = leaf_id;
}

void Block::set_bucket_idx(uint32_t bucket_idx)
{
	this->bucket_idx = bucket_idx;
}

void Block::set_data(const std::array<uint8_t, BYTES_PER_BLOCK>& data)
{
	this->data = data;
}

