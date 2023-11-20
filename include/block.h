#ifndef PORAM_BLOCK_H
#define PORAM_BLOCK_H

#include <cstdint>
#include <cstring>
#include <random>

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/aes.h>

#define BYTES_PER_DATA 1022
#define BLOCKS_PER_BUCKET 3
#define BYTES_PER_BLOCKID sizeof(uint16_t)
#define BLOCK_SIZE (BYTES_PER_BLOCKID + BYTES_PER_DATA)
#define BUCKETS_PER_TREE 0xffff

class Block {
public:
	Block();
	Block(const Block &block);
	Block(uint16_t block_id, const uint8_t *data, uint32_t data_size);

	void swap(Block &block);

	uint16_t get_block_id() { return bytes.data_dec.block_id; }
	uint8_t *get_decrypted_data() { return bytes.data_dec.data; }
	uint8_t *get_encrypted_data() { return bytes.data_enc.data; }

	void set_block_id(uint16_t block_id) { bytes.data_dec.block_id = block_id; }
	void set_decrypted_data(const uint8_t *data, uint32_t data_size);
	void set_encrypted_data(const uint8_t *data, uint32_t data_size);
	void set_decrypted_random_data();

	int encrypt(const uint8_t *key, const uint8_t *iv);	
	int decrypt(const uint8_t *key, const uint8_t *iv);

	bool is_encrypted;
private:
	union bytes {
		struct __attribute__((__packed__)) {
			uint16_t block_id;
			uint8_t data[BYTES_PER_DATA];
		} data_dec;
		struct __attribute__((__packed__)) {
			uint8_t data[BLOCK_SIZE];
		} data_enc;
	} bytes;
};

#endif /* PORAM_BLOCK_H */
