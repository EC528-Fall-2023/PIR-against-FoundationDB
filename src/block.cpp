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
	if (data_size < BYTES_PER_DATA) {
		memcpy(bytes.data_dec.data, data, data_size);
		memset(bytes.data_dec.data + data_size, 0, BYTES_PER_DATA - data_size);
	} else {
		memcpy(bytes.data_dec.data, data, BYTES_PER_DATA);
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
	if (data_size < BYTES_PER_DATA) {
		memcpy(bytes.data_dec.data, data, data_size);
		memset(bytes.data_dec.data + data_size, 0, BYTES_PER_DATA - data_size);
	} else {
		memcpy(bytes.data_dec.data, data, BYTES_PER_DATA);
	}
}

void Block::set_encrypted_data(const uint8_t *data, uint32_t data_size)
{
	if (data_size < BLOCK_SIZE) {
		memcpy(bytes.data_enc.data, data, data_size);
		memset(bytes.data_enc.data + data_size, 0, BLOCK_SIZE - data_size);
	} else {
		memcpy(bytes.data_enc.data, data, BLOCK_SIZE);
	}
}

void Block::set_decrypted_random_data()
{
	std::random_device generator;
	std::uniform_int_distribution<uint8_t> random_byte(0x00, 0xFF);

	for (uint32_t i = 0; i < BYTES_PER_DATA; ++i) {
		bytes.data_dec.data[i] = random_byte(generator);
	}
}

int Block::encrypt(const uint8_t *key, const uint8_t *iv)
{
	EVP_CIPHER_CTX *ctx;
	int len;
	int ciphertext_len;

	// initialize cipher context
	if (!(ctx = EVP_CIPHER_CTX_new()))
		return -1;

	// initialize encryption operation + cipher type
	if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
		return -1;

	// don't padd the blocks
	EVP_CIPHER_CTX_set_padding(ctx, 0);
 
 	// does encryption
 	uint8_t out_buffer[BLOCK_SIZE];
	if (1 != EVP_EncryptUpdate(ctx, out_buffer, &len, bytes.data_enc.data, sizeof(union bytes)))
		return -1;

	ciphertext_len = len;

	// does final parts of encryption
	if (1 != EVP_EncryptFinal_ex(ctx, out_buffer + len, &len))
		return -1;

	ciphertext_len += len;
	EVP_CIPHER_CTX_free(ctx);
	is_encrypted = true;
	memset(bytes.data_enc.data, 0, BLOCK_SIZE);
	memcpy(bytes.data_enc.data, out_buffer, sizeof(out_buffer));

	return ciphertext_len;
}

int Block::decrypt(const uint8_t *key, const uint8_t *iv)
{
	EVP_CIPHER_CTX *ctx;
	int len;
	int plaintext_len;

	// initialize cipher context
	if (!(ctx = EVP_CIPHER_CTX_new()))
		return -1;

	// initialize decryption operation + cipher type
	if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
		return -1;

	// don't padd the blocks
	EVP_CIPHER_CTX_set_padding(ctx, 0);
 
	// does decryption
 	uint8_t out_buffer[BLOCK_SIZE];
	if (1 != EVP_DecryptUpdate(ctx, out_buffer, &len, bytes.data_enc.data, sizeof(union bytes)))
		return -1;

	plaintext_len = len;

	// does final parts of decryption
	if (1 != EVP_DecryptFinal_ex(ctx, out_buffer + len, &len))
		return -1;

	plaintext_len += len;
	EVP_CIPHER_CTX_free(ctx);
	is_encrypted = false;
	memset(bytes.data_enc.data, 0, BLOCK_SIZE);
	memcpy(bytes.data_enc.data, out_buffer, sizeof(out_buffer));

	return plaintext_len;
}

