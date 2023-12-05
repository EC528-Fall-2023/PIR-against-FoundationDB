#ifndef PORAM_BLOCK_H
#define PORAM_BLOCK_H

#include <cstdint>
#include <cstring>
#include <random>

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/aes.h>

/* YOU CAN MODIFY */
#ifndef BLOCK_SIZE
	#define BLOCK_SIZE 1024		/* MUST BE A MULTIPLE OF 16 */
#endif

#ifndef BLOCKS_PER_BUCKET
	#define BLOCKS_PER_BUCKET 1	/* MUST BE RELATIVELY SMALL */
#endif

#ifndef TREE_LEVELS
	#define TREE_LEVELS 8		/* MUST BE TRUE: (2^TREE_LEVELS - 1) * BLOCKS_PER_BUCKET <= 2^(bits(blkid_t)) - 1 */
#endif

/* DO NOT MODIFY */
#if TREE_LEVELS <= 1
	#error "TREE_LEVELS must be > 1"
#elif TREE_LEVELS <= 8
	#define blkid_t uint8_t
#elif TREE_LEVELS <= 16
	#define blkid_t uint16_t
#elif TREE_LEVELS <= 32
	#define blkid_t uint32_t
#elif TREE_LEVELS <= 64
	#define blkid_t uint64_t
#else
	#error "TREE_LEVELS must be <= 64"
#endif

#define BYTES_PER_DATA (BLOCK_SIZE - sizeof(blkid_t))
#define BUCKETS_PER_TREE (0xffffffffffffffff >> (64 - TREE_LEVELS))

class Block {
public:
	Block();
	Block(const Block &block);
	Block(blkid_t block_id, const uint8_t *data, uint32_t data_size);

	void swap(Block &block);

	blkid_t get_block_id() { return bytes.data_dec.block_id; }
	uint8_t *get_decrypted_data() { return bytes.data_dec.data; }
	uint8_t *get_encrypted_data() { return bytes.data_enc.data; }

	void set_block_id(blkid_t block_id) { bytes.data_dec.block_id = block_id; }
	void set_decrypted_data(const uint8_t *data, uint32_t data_size);
	void set_encrypted_data(const uint8_t *data, uint32_t data_size);
	void set_decrypted_random_data();

	int encrypt(const uint8_t *key, const uint8_t *iv);	
	int decrypt(const uint8_t *key, const uint8_t *iv);

	bool is_encrypted;
private:
	union bytes {
		struct __attribute__((__packed__)) {
			blkid_t block_id;
			uint8_t data[BYTES_PER_DATA];
		} data_dec;
		struct __attribute__((__packed__)) {
			uint8_t data[BLOCK_SIZE];
		} data_enc;
	} bytes;
};

#endif /* PORAM_BLOCK_H */
