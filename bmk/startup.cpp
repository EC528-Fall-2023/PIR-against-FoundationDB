#include "single_client.h"
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <chrono>
#include <wait.h>

int main()
{
	int out_file;
	std::string out_file_path = "bmk_out/startup_" + std::to_string(BLOCK_SIZE) + "_" + std::to_string(BLOCKS_PER_BUCKET) + "_" + std::to_string(TREE_LEVELS) + ".txt";
	if ( (out_file = open(out_file_path.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0666)) == -1 ) {
		perror("open");
		return -1;
	}

	SingleClient client;
	
	auto tic = std::chrono::high_resolution_clock::now();
	client.initialize();
	auto toc = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> elapsed = toc - tic;

	dprintf(out_file, "%f\n", elapsed.count());

	client.shutdown();

	return 0;
}
