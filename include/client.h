#ifndef PORAM_CLIENT_H
#define PORAM_CLIENT_H

#include <string>

class PathOramClient {
public:
	PathOramClient(const std::string &server_ip);
	~PathOramClient();

	int put(const std::string &key_name, const std::string &value);
	int get(const std::string &key_name, std::string &value);
	int read_range(const std::string &begin_key_name, const std::string &end_key_name);
	int clear_range(const std::string &begin_key_name, const std::string &end_key_name);

private:
	
};


#endif
