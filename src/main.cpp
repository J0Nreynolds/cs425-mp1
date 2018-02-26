#include <iostream>
#include "server.h"
#include "client.h"

using namespace std;

int main(int argc, char **argv) {
    Server* s = new Server( (std::string) argv[1]);
    Client* c = new Client();
}
