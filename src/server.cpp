#include "server.h"

void nick(){
	printf("i love honey\n");
}

/**
 * Server constructor
 * Initializes max # of clients based on config file
 */
Server::Server(std::string port){
	std::ifstream config_file("multicast.config");
	std::string line;
	this->max_clients = 0;
	while (std::getline(config_file, line)){ this->max_clients++; }
	printf("Counted %d total clients\n", this->max_clients);
	start(port);
	begin_processing(&nick);
}

/**
 * Begins listening for incoming connections on the passed port
 */
void Server::start(std::string port){
	int error;
	this->socket_fd = socket(AF_INET, SOCK_STREAM, 0);;
    fcntl(this->socket_fd, F_SETFL, fcntl(this->socket_fd, F_GETFL, 0) | O_NONBLOCK); // Set to non-blocking

	struct addrinfo hints, *result;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET; // IPv4
	hints.ai_socktype = SOCK_STREAM; // Streaming uses TCP
	hints.ai_flags = AI_PASSIVE; // Servers are passive

	error = getaddrinfo(NULL, port.c_str(), &hints, &result);
	if (error != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
		exit(1);
	}

	if (bind(this->socket_fd, result->ai_addr, result->ai_addrlen) != 0) {
		perror("bind()");
		exit(1);
	}

	if (listen(this->socket_fd, this->max_clients) != 0) {
		perror("listen()");
		exit(1);
	}

	freeaddrinfo(result);
}

void Server::begin_processing(void (* func_pt)(void)){
	// while(true){
	// 	//fprintf(stderr, "Waiting for connection...\n");
	// 	int client_fd = accept(this->socket_fd, NULL, NULL);
	// 	if(client_fd >= 0){
	// 		clients_count += 1;
	//
	// 		int flags = fcntl(client_fd, F_GETFL, 0);
	// 		fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
	//
	// 		//set up epoll stuff
	// 		struct epoll_event event;
	// 		memset(&event, 0, sizeof(struct epoll_event));
	// 		event.events = EPOLLIN | EPOLLOUT | EPOLLET;  // EPOLLIN==read, EPOLLOUT==write
	// 		event.data.fd = client_fd;
	// 		int s = epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &event);
	// 		if(s == -1){
	// 			perror("epoll_ctl");
	// 			cleanup();
	// 			exit(1);
	// 		}
	// 	}
	// 	int n = epoll_wait(epfd, events, this->max_clients, 0);
	// 	if(n < 0){
	// 		fprintf(stderr, "error in epoll_wait\n");
	// 	}
	// 	for(int i = 0; i < n; i++){
	// 		int fd = events[i].data.fd;
	// 		process_socket(fd);
	// 	}
	// }
	func_pt();
}
