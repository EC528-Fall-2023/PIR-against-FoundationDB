#include "tree.h"

Tree::Tree()
{
	
}

Tree::Tree(uint32_t initial_size)
{
	buckets.resize(initial_size);
	for (Bucket &bucket : buckets) {
		
	}
}

Tree::Tree(const std::vector<Bucket>& buckets)
{
	this->buckets = buckets;
}


void Tree::add_bucket(Bucket bucket)
{
	buckets.push_back(bucket);
}


std::vector<Bucket> Tree::get_buckets()
{
	return buckets;
}

void Tree::set_buckets(const std::vector<Bucket>& buckets)
{
	this->buckets = buckets;
}
