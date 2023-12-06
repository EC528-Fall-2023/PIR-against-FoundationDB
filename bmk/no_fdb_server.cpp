#include <sys/socket.h>
#include <arpa/inet.h>
#define FDB_API_VERSION 710
//#include <foundationdb/fdb_c.h>


#ifdef DEBUG
	#include <error.h>
	#define ERROR(function) error_at_line(0, errno, __FILE__, __LINE__, "%s: %s failed", __func__, function);
#else
	#define ERROR(function)
#endif

inline int update_fdb();

int main()
{
	int server_socket;
	struct sockaddr_in server_addr, client_addr;

	socklen_t client_addr_len = sizeof(client_addr);

	pthread_t network_thread;
	if (setup_socket(server_socket, server_addr, network_thread) != 0) {
		ERROR("setup_socket");
		exit(EXIT_FAILURE);
	}

	while (1) {
		LISTEN:
		if ( (client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_addr_len)) < 0 ) {
			ERROR("accept");
			continue;
		}

	// send operation type
	uint8_t op = PUT;
	if (send(socket_fd, &op, sizeof(op), 0) != sizeof(op)) {
		ERROR("send");
		return -1;
	}
		while (1) {
			// receive request_id
			uint8_t op;
            if (recv(client_socket, &op, sizeof(op), 0) != sizeof(op)) {
                ERROR("recv")
				close_connection();
				goto LISTEN;
            }

			switch (op)
			case PUT:
                // recv key from client
                uint32_t key_size;
                if (recv(socket_fd, &key_size, sizeof(key_size), 0) != sizeof(key_size)) {
                    ERROR("send");
                    return -1;
                }
                std::string key;
                key.resize(key_size);
                if (recv(socket_fd, key.c_str(), key_size, MSG_WAITALL) != key_size) {
                    ERROR("send");
                    return -1;
                }
                
                // recv data from client
                std::array<uint8_t, BYTES_PER_DATA> data = {0};
                if (recv(socket_fd, data.data(), BYTES_PER_DATA, MSG_WAITALL) != BYTES_PER_DATA) {
                    ERROR("send");
                    return -1;
                }

                if (update_fdb() != 0) {
                    ERROR("update_fdb")
                    return -1;
                }

                // confirm put success
                uint8_t confirmation = 0;
                if (recv(socket_fd, &confirmation, sizeof(confirmation), 0) != sizeof(confirmation)) {
                    ERROR("recv");
                    return -1;
                }
                if (confirmation != 1) {
                    ERROR("confirmation");
                    return -1;
                }
                break;
            default:
                shutdown(server_socket, SHUT_RDWR);
                if (fdb_stop_network() != 0) {
                    ERROR("fdb_stop_network");
                    exit(EXIT_FAILURE);
                }
                pthread_join(network_thread, NULL);
                fdb_database_destroy(db);

                exit(0);
			}



		}
	}

	shutdown(server_socket, SHUT_RDWR);

	if (fdb_stop_network() != 0) {
		ERROR("fdb_stop_network");
		exit(EXIT_FAILURE);
	}
	pthread_join(network_thread, NULL);
	fdb_database_destroy(db);

	return 0;
}