#include "client.h"

/**
 * Client class constructor
 * Initializes hashmap mapping process ids to process info (ip, port, status)
 */
Client::Client(){
	std::ifstream config_file("multicast.config");
	std::string line;
	while (std::getline(config_file, line)){
	    std::istringstream iss(line);
	    unsigned int process_id;
		std::string ip, port;

	    if (!(iss >> process_id >> ip >> port)) {
			std::cout << "Error parsing config file!" << std::endl;
			break;
		} // error

		// Create connection struct instance for hashmap
		struct connection connection_info;
		connection_info.ip = ip;
		connection_info.port = port;
		connection_info.status = "NONE";

		// Create pair for insertion
		std::pair<unsigned int, struct connection> entry(process_id, connection_info);
		//Insert
		this->connections.insert(entry);
	}

	// Print filled map
	for (auto& x: this->connections){
		struct connection info = x.second;
		std::cout << x.first << ": " << info.ip << "\t" << info.port << std::endl;
	}
	connect_to_server("127.0.0.1", "1234");
}

/**
 * Connects to server and returns the socket file descriptor upon success
 */
int Client::connect_to_server(char* host, char* port){
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0); // Get a file descriptor for a streaming (TCP) socket
	if(sock_fd < 0) exit(1);

	struct addrinfo hints, *result; // addrinfo instances to hold host information
	memset(&hints, 0, sizeof(struct addrinfo)); //clear memory
	hints.ai_family = AF_INET; /* IPv4 only */
	hints.ai_socktype = SOCK_STREAM; /* TCP */

	int s = getaddrinfo(host, port, &hints, &result); //Get host information for connect
	if(s != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(1);
	}

	if(connect(sock_fd, result->ai_addr, result->ai_addrlen) == -1){ // Connect to host using sock_fd and resulting addrinfo
		perror("connect");
		exit(1);
    }
	freeaddrinfo(result); // free memory pointed to by result
	return sock_fd;
}
