#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <chrono>
#include <unistd.h>
#include <pthread.h>
#include <fstream>
#include <stdlib.h>

#define FDB_API_VERSION 710
#include <foundationdb/fdb_c.h>

void *run_network(void *arg)
{
	if (fdb_run_network() != 0) {
		_exit(1);
	}
	return arg;
}

int main(int argc, char **argv)
{
	int out_file;
	std::string out_file_path = "bmk_out/without.txt";
	if ( (out_file = open(out_file_path.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0666)) == -1 ) {
		perror("open");
		return -1;
	}
	auto tic = std::chrono::high_resolution_clock::now();
	if (argc != 3 || argv[1][0] != '-' || argv[1][2] != '\0' || (argv[1][1] != 'r' && argv[1][1] != 'w' && argv[1][1] != 'c') || (argv[1][1] == 'w' && memchr(argv[2], '=', strlen(argv[2])) == NULL)) {
		printf("Usage: ./test [-r <KEY>] [-w <KEY>=<VALUE>] [-c <KEY>]\n");
		return 0;
	}

	char option = argv[1][1];

	// initial setups
	fdb_select_api_version(FDB_API_VERSION);
	if (fdb_setup_network() != 0) {
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

	// handle reads, writes, clears
	struct FDB_future *status = NULL;
	if (option == 'w') {
		char delimiter[2] = "=";
		char *key = strtok(argv[2], delimiter);
		char *value = strtok(NULL, delimiter);
		fdb_transaction_set(tr, (const uint8_t *) key, strlen(key), (const uint8_t *) value, strlen(value));
		status = fdb_transaction_commit(tr);
	} else if (option == 'r') {
		status = fdb_transaction_get(tr, (const uint8_t *) argv[2], strlen(argv[2]), 0);
	} else {
		fdb_transaction_clear(tr, (const uint8_t *) argv[2], strlen(argv[2]));
		status = fdb_transaction_commit(tr);
	}
	if ((fdb_future_block_until_ready(status)) != 0) {
		perror("fdb_future_block_until_ready: failed");
		return 1;
	}
	if (fdb_future_get_error(status)) {
		perror("fdb_future_is_error: its an error...");
		return 1;
	}
	if (option == 'r') {
		fdb_bool_t out_present = 0;
		const uint8_t *out_value = NULL;
		int out_value_length = 0;
		if (fdb_future_get_value(status, &out_present, &out_value, &out_value_length) == 0) {
			if (out_present == 0)
				printf("%s does not exist\n", argv[2]);
			else
				write(1, out_value, out_value_length);
		} else {
			perror("fdb_future_get_value: failed");
			return 1;
		}
	}
	fdb_future_destroy(status);

	// clean up
	if (fdb_stop_network() != 0) {
		perror("fdb_stop_network: failed");
		return 1;
	}
	if (pthread_join(network_thread, NULL) != 0) {
		perror("pthread_join: failed");
		return 1;
	}
	fdb_transaction_destroy(tr);
	fdb_database_destroy(db);

	auto toc = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> elapsed = toc - tic;

	dprintf(out_file, "%f\n", elapsed.count());

	return 0;
}
