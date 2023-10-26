#include "block.h"

Block::Block()
{
	leaf_id = 0;
	// data = random
}

Block::Block(uint16_t leaf_id, const std::array<uint8_t, BYTES_PER_BLOCK>& data)
{
	this->leaf_id = leaf_id;
	this->data = data;
}

uint16_t Block::get_leaf_id()
{
	return leaf_id;
}

std::array<uint8_t, BYTES_PER_BLOCK> Block::get_data()
{
	return data;
}

void Block::set_leaf_id(uint16_t leaf_id)
{
	this->leaf_id = leaf_id;
}

void Block::set_data(const std::array<uint8_t, BYTES_PER_BLOCK>& data)
{
	this->data = data;
}

