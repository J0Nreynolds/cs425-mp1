#include "client.h"

/**
 * Client class constructors
 */
Client::Client():
	connected(false),
	socket_fd(-1)
{
}

Client::Client(std::string ip, std::string port, unsigned int id):
	connected(false),
	socket_fd(-1)
{
   connect_to_server(ip, port, id);
}

/**
 * Connects to server and returns the socket file descriptor upon success
 */
int Client::connect_to_server(std::string ip, std::string port, unsigned int id){
	this->socket_fd = socket(AF_INET, SOCK_STREAM, 0); // Get a file descriptor for a streaming (TCP) socket
	if(socket_fd < 0) exit(1);

	struct addrinfo hints, *result; // addrinfo instances to hold host information
	memset(&hints, 0, sizeof(struct addrinfo)); //clear memory
	hints.ai_family = AF_INET; /* IPv4 only */
	hints.ai_socktype = SOCK_STREAM; /* TCP */

	int s = getaddrinfo(ip.c_str(), port.c_str(), &hints, &result); //Get host information for connect
	if(s != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(1);
	}

	if(connect(socket_fd, result->ai_addr, result->ai_addrlen) == -1){ // Connect to host using sock_fd and resulting addrinfo
		perror("connect");
		return -1;
    }
	this->connected = true;
	
	int written = write_all_to_socket(socket_fd, (char *) &id, sizeof(int));
	if(written != sizeof(int)){
		std::cout << "Error: " << written << " bytes written instead of " << sizeof(int) << std::endl;
	}

	freeaddrinfo(result); // free memory pointed to by result
	return socket_fd;
}

int Client::get_socket_fd(){
	return this->socket_fd;
}

bool Client::is_connected(){
	return this->connected;
}
