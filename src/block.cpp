#include "block.h"

Block::Block()
{
	memset(&bytes, 0, sizeof(union bytes));
	is_encrypted = false;
}

Block::Block(const Block &block)
{
	memcpy(&bytes, &block.bytes, sizeof(union bytes));
	is_encrypted = block.is_encrypted;
}

Block::Block(uint16_t block_id, const uint8_t *data, uint32_t data_size)
{
	bytes.data_dec.block_id = block_id;
	if (data_size < BYTES_PER_BLOCK) {
		memcpy(bytes.data_dec.data, data, data_size);
		memset(bytes.data_dec.data + data_size, 0, BYTES_PER_BLOCK - data_size);
	} else {
		memcpy(bytes.data_dec.data, data, BYTES_PER_BLOCK);
	}
	is_encrypted = false;
}

void Block::swap(Block &block)
{
	union bytes temp;
	memset(&temp, 0, sizeof(union bytes));
	memcpy(&temp, &bytes, sizeof(union bytes));
	memcpy(&bytes, &block.bytes, sizeof(union bytes));
	memcpy(&block.bytes, &temp, sizeof(union bytes));
}

void Block::set_decrypted_data(const uint8_t *data, uint32_t data_size)
{
	if (data_size < BYTES_PER_BLOCK) {
		memcpy(bytes.data_dec.data, data, data_size);
		memset(bytes.data_dec.data + data_size, 0, BYTES_PER_BLOCK - data_size);
	} else {
		memcpy(bytes.data_dec.data, data, BYTES_PER_BLOCK);
	}
}

void Block::set_encrypted_data(const uint8_t *data, uint32_t data_size)
{
	if (data_size < BYTES_PER_BLOCK + BLOCK_ID_SIZE) {
		memcpy(bytes.data_enc.data, data, data_size);
		memset(bytes.data_enc.data + data_size, 0, BYTES_PER_BLOCK + BLOCK_ID_SIZE - data_size);
	} else {
		memcpy(bytes.data_enc.data, data, BYTES_PER_BLOCK + BLOCK_ID_SIZE);
	}
}

void Block::set_decrypted_random_data()
{
	std::random_device generator;
	std::uniform_int_distribution<uint8_t> random_byte(0x00, 0xFF);

	for (uint32_t i = 0; i < BYTES_PER_BLOCK; ++i) {
		bytes.data_dec.data[i] = random_byte(generator);
	}
}

int Block::encrypt(const uint8_t *key, const uint8_t *iv)
{
	// put encryption here
	is_encrypted = true;
}

int Block::decrypt(const uint8_t *key, const uint8_t *iv)
{
	// put decryption here
	is_encrypted = false;
}

