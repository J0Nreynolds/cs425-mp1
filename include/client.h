#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netdb.h>

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <sstream>

#include "common.h"

class Client {

public:
    Client();
    Client(std::string ip, std::string port, unsigned int id);
    int connect_to_server(std::string ip, std::string port, unsigned int id);
    int get_socket_fd();
    bool is_connected();

private:
    int socket_fd;
    bool connected;

};

#endif
