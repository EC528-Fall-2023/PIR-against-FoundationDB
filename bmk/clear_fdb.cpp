#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define FDB_API_VERSION 710

#include <foundationdb/fdb_c.h>

void *run_network(void *arg)
{
	if (fdb_run_network() != 0) {
		perror("fdb_run_network");
		_exit(-1);
	}
	return arg;
}

struct FDB_database *db = NULL;
struct FDB_transaction *tr = NULL;
struct FDB_future *status = NULL;

int main()
{
	fdb_select_api_version(FDB_API_VERSION);

	if (fdb_setup_network() != 0) {
		perror("fdb_setup_network");
		return -1;
	}

	pthread_t network_thread;
	if (pthread_create(&network_thread, NULL, run_network, NULL) != 0) {
		perror("pthread_create");
		return -1;
	}

	if (fdb_create_database(NULL, &db) != 0) {
		perror("fdb_create_database");
		return -1;
	}

	if (fdb_database_create_transaction(db, &tr) != 0) {
		perror("fdb_database_create_transaction");
		return -1;
	}

	const uint8_t begin_key = 0x00;
	const uint8_t end_key = 0xff;
	fdb_transaction_clear_range(tr, &begin_key, 1, &end_key, 1);

	status = fdb_transaction_commit(tr);
	if ((fdb_future_block_until_ready(status)) != 0) {
		perror("fdb_future_block_until_ready");
		return -1;
	}
	if (fdb_future_get_error(status) != 0) {
		perror("fdb_future_get_error");
		return -1;
	}

	fdb_future_destroy(status);
	fdb_transaction_destroy(tr);
	fdb_database_destroy(db);

	return 0;
}
