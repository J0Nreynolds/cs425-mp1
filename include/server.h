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

#include <fstream>

class Server {

public:
    Server(std::string port);

private:
    void start(std::string port);
    void begin_processing(void (* func_pt)(void));
    int max_clients;
    int socket_fd;

};

#endif
