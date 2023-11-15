#ifndef PORAM_BLOCK_H
#define PORAM_BLOCK_H

#include <cstdint>
#include <cstring>
#include <random>

#define BYTES_PER_BLOCK 1024
#define BLOCKS_PER_BUCKET 3
#define BLOCK_ID_SIZE sizeof(uint16_t)

class Block {
public:
	Block();
	Block(const Block &block);
	Block(uint16_t block_id, const uint8_t *data, uint32_t data_size);

	void swap(Block &block);

	uint16_t get_block_id();
	uint8_t *get_decrypted_data();
	uint8_t *get_encrypted_data();

	void set_block_id(uint16_t block_id);
	void set_decrypted_data(const uint8_t *data, uint32_t data_size);
	void set_encrypted_data(const uint8_t *data, uint32_t data_size);
	void set_decrypted_random_data();

	int encrypt(uint8_t *key, uint8_t *iv);	
	int decrypt(uint8_t *key, uint8_t *iv);	

	bool is_encrypted;
private:
	union bytes {
		struct __attribute__((__packed__)) {
			uint16_t block_id;
			uint8_t data[BYTES_PER_BLOCK];
		} data_dec;
		struct __attribute__((__packed__)) {
			uint8_t data[BLOCK_ID_SIZE + BYTES_PER_BLOCK];
		} data_enc;
	} bytes;
};

#endif /* PORAM_BLOCK_H */
