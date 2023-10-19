#ifndef PORAM_TREE_H
#define PORAM_TREE_H

#include "bucket.h"
#include <vector>

// implement tree as an array (non sorting heap)
class Tree {
public:
	Tree();
	Tree(uint32_t initial_size);
	Tree(const std::vector<Bucket>& buckets);

	void add_bucket(Bucket bucket);

	std::vector<Bucket> get_buckets();
	void set_buckets(const std::vector<Bucket>& buckets);
private:
	std::vector<Bucket> buckets;
	uint32_t levels;
};

#endif /* PORAM_TREE_H */
