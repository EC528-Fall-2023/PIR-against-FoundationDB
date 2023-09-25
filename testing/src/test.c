#define NULL 0
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

#define FDB_API_VERSION 630
#include <foundationdb/fdb_c.h>

void *run_network(void *arg)
{
	fdb_run_network();
	return NULL;
}

int main(int argc, char **argv)
{
	fdb_select_api_version(FDB_API_VERSION);
	fdb_setup_network();

	pthread_t thread;
	pthread_create(&thread, NULL, run_network, NULL);

	// create a database
	struct FDB_database *db = NULL;
	if (fdb_create_database(NULL, &db)) {
		perror("fdb_create_database: failed");
		return 1;
	}

	// create a transaction
	struct FDB_transaction *tr = NULL;
	if (fdb_database_create_transaction(db, &tr)) {
		perror("fdb_database_create_transaction: failed");
		fdb_database_destroy(db);
		return 1;
	}

	// writting: db[Dan Lambright] = mentor
	if (argc == 2 && argv[1][0] == '-' && argv[1][1] == 'w' && argv[1][2] == '\0') {
		fdb_transaction_set(tr, (const uint8_t *)"Dan Lambright", 14, (const uint8_t *)"mentor", 7);
		struct FDB_future *status = fdb_transaction_commit(tr);
		if ((fdb_future_block_until_ready(status)) != 0) {
			perror("fdb_future_block_until_ready: failed");
			fdb_transaction_destroy(tr);
			fdb_database_destroy(db);
			return 1;
		}
		if (fdb_future_get_error(status)) {
			perror("fdb_future_is_error: its an error...");
			fdb_future_destroy(status);
			fdb_transaction_destroy(tr);
			fdb_database_destroy(db);
			return 1;
		}
		fdb_future_destroy(status);
	}

	// reading: value = db[Dan Lambright]
	if (argc == 2 && argv[1][0] == '-' && argv[1][1] == 'r' && argv[1][2] == '\0') {
		struct FDB_future *result = fdb_transaction_get(tr, (const uint8_t *)"Dan Lambright", 14, 0);
		if ((fdb_future_block_until_ready(result)) != 0) {
			perror("fdb_future_block_until_ready: failed");
			fdb_future_destroy(result);
			fdb_transaction_destroy(tr);
			fdb_database_destroy(db);
			return 1;
		}
		fdb_bool_t out_present = 0;
		const uint8_t *out_value = NULL;
		int out_value_length = 0;
		if (fdb_future_get_value(result, &out_present, &out_value, &out_value_length) == 0) {
			// success
			if (out_present != 0) {
				write(1, out_value, out_value_length);
			}
		} else {
			// fail
		}
	}

	if (argc == 2 && argv[1][0] == '-' && argv[1][1] == 'c' && argv[1][2] == '\0') {
		fdb_transaction_clear(tr, (const uint8_t *)"Dan Lambright", 14);
		struct FDB_future *status = fdb_transaction_commit(tr);
		if ((fdb_future_block_until_ready(status)) != 0) {
			perror("fdb_future_block_until_ready: failed");
			fdb_transaction_destroy(tr);
			fdb_database_destroy(db);
			return 1;
		}
		if (fdb_future_get_error(status)) {
			perror("fdb_future_is_error: its an error...");
			fdb_future_destroy(status);
			fdb_transaction_destroy(tr);
			fdb_database_destroy(db);
			return 1;
		}
		fdb_future_destroy(status);
	}

	// clean up
	fdb_transaction_destroy(tr);
	fdb_database_destroy(db);

	return 0;
}
