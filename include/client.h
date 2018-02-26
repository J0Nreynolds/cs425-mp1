#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netdb.h>

#include <fstream>
#include <unordered_map>
#include <iostream>
#include <sstream>

class Client {

    struct connection {   // Declare connection struct type
        std::string ip;
        std::string port;
        std::string status;
    };

public:
    Client();

private:
    int connect_to_server(char* host, char* port);
    std::unordered_map<unsigned int, struct connection> connections;
};

#endif
