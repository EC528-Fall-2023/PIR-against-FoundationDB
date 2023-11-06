#include "block.h"

static std::random_device generator;
static std::uniform_int_distribution<uint8_t> random_byte(0x00, 0xFF);

Block::Block()
{
	block_id = 0;
	for (uint8_t &byte: data) {
		byte = random_byte(generator);
	}
}

Block::Block(const Block &block) {
	this->block_id = block.block_id;
	this->data = block.data;
}

Block::Block(uint16_t block_id, const std::array<uint8_t, BYTES_PER_BLOCK>& data)
{
	this->block_id = block_id;
	this->data = data;
}

uint16_t Block::get_block_id()
{
	return block_id;
}

std::array<uint8_t, BYTES_PER_BLOCK> &Block::get_data()
{
	return data;
}

void Block::set_block_id(uint16_t block_id)
{
	this->block_id = block_id;
}

void Block::set_data(const std::array<uint8_t, BYTES_PER_BLOCK>& data)
{
	this->data = data;
}

void Block::set_random_data()
{
	for (uint8_t &byte: data) {
		byte = random_byte(generator);
	}
}
