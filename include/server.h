#ifndef SERVER_H
#define SERVER_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "common.h"

class Server {

public:
    Server(std::string port, int max_clients);
    void start(std::string port, int max_clients);
    int get_socket_fd();
    int accept_client();
    void close();

private:
    void close_clients();
    int socket_fd;
    std::vector<int> client_fds;

};

#endif
