#include "block.h"
#include <iostream>
#include <cstring>

#define FDB_API_VERSION 710
#include <foundationdb/fdb_c.h>

void *run_network(void *arg)
{
	if (fdb_run_network() != 0) {
		std::cerr << "fdb_run_network: failed\n";
		exit(EXIT_FAILURE);
	}
	return arg;
}

int main()
{
	// initial setups
	fdb_select_api_version(FDB_API_VERSION);
	if (fdb_setup_network() != 0) {
		perror("fdb_setup_network: failed\n");
		return 1;
	}
	pthread_t network_thread;
	if (pthread_create(&network_thread, NULL, run_network, NULL) != 0) {
		perror("pthread_create: failed");
		return 1;
	}	

	// create a database
	struct FDB_database *db = NULL;
	if (fdb_create_database(NULL, &db) != 0) {
		perror("fdb_create_database: failed");
		return 1;
	}

	// create a transaction
	struct FDB_transaction *tr = NULL;
	if (fdb_database_create_transaction(db, &tr) != 0) {
		perror("fdb_database_create_transaction: failed");
		return 1;
	}

	// set data
	uint16_t tree_index = 1;
	uint16_t leaf_id = 1;
	std::string value = "was here";

	uint8_t temp[3];
	temp[0] = 0;
	temp[1] = (tree_index & 0xff00) >> 8;
	temp[2] = tree_index & 0x00ff;
	std::array<uint8_t, (sizeof(leaf_id) + BYTES_PER_BLOCK) * BLOCKS_PER_BUCKET> bucket;
	bucket.fill(0);
	memcpy(bucket.data(), &leaf_id, sizeof(leaf_id));
	memcpy(bucket.data() + sizeof(leaf_id), value.data(), value.size());
	fdb_transaction_set(tr, temp, sizeof(temp), (const uint8_t *) bucket.data(), bucket.size());

	// commit data
	struct FDB_future *status = fdb_transaction_commit(tr);
	if ((fdb_future_block_until_ready(status)) != 0) {
		perror("fdb_future_block_until_ready: failed");
		return 1;
	}
	if (fdb_future_get_error(status)) {
		perror("fdb_future_is_error: its an error...");
		return 1;
	}

	// clean up
	fdb_future_destroy(status);
	fdb_transaction_destroy(tr);
	if (fdb_stop_network() != 0) {
		perror("fdb_stop_network: failed");
		return 1;
	}
	if (pthread_join(network_thread, NULL) != 0) {
		perror("pthread_join: failed");
		return 1;
	}	
	fdb_database_destroy(db);

	return 0;
}
