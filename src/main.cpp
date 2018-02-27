#include <iostream>
#include <list>
#include <thread>
#include <mutex>

#include <signal.h>

#include "server.h"
#include "client.h"
#include "common.h"

using namespace std;

struct connection {   // Declare connection struct type
    std::string ip;
    std::string port;
    Client* client;
};

std::unordered_map<unsigned int, struct connection> processes;
std::list<unsigned int> unprocessed_fds;
std::unordered_map<unsigned int, unsigned int> fd_to_id;
unsigned int process_id;
Server* s;
bool end_session = false;

/**
 * Initializes the clients for this process and attempts to connect to other servers.
 */
void parse_config(){
	std::ifstream config_file("multicast.config");
	std::string line;
	while (std::getline(config_file, line)){
	    std::istringstream iss(line);
	    unsigned int pid;
		std::string ip, port;
        // Get three arguments from each line
	    if (!(iss >> pid >> ip >> port)) { // error
			std::cout << "Error parsing config file!" << std::endl;
			break;
		}

		// Create connection struct instance for hashmap
		struct connection connection_info;
		connection_info.ip = ip;
		connection_info.port = port;
		connection_info.client = NULL;

		// Create pair for insertion
		std::pair<unsigned int, struct connection> entry(pid, connection_info);
		//Insert
		processes.insert(entry);
	}
}

/**
 *
 */
void process_fds(){
    unsigned int len = unprocessed_fds.size();
    for(int i = 0; i < len; i ++){
        int fd = unprocessed_fds.front();
        unprocessed_fds.pop_front();
        unsigned int pid = 0;
        int read_bytes = read_all_from_socket(fd, (char *) &pid, sizeof(int));
    	if(read_bytes != sizeof(int)){
            unprocessed_fds.push_back(fd); // Add back for an attempt later.
    		// std::cout << "Error: " << read_bytes << " bytes read instead of " << sizeof(int) << std::endl;
    	}
        else {
            std::cout << "Got id: " << pid << " from client with fd: " << fd << std::endl;
            if(processes[pid].client != NULL){
                if(!processes[pid].client->is_connected()){
                    // processes[pid].client->connect_to_server(processes[pid].ip, processes[pid].port, pid);
                }
            }
        }
    }
}

void check_messages(){

}

void process_input(){
    std::string command, dest_string, message;
    int destination;

    std::string line;
    std::getline( std::cin, line );
    if(line.empty()){
        return;
    }

    int space1_idx = line.find(' ', 0);
    if(space1_idx == std::string::npos){ return; } // incorrect command format
    int space2_idx = line.find(' ', space1_idx+1);
    if(space2_idx == std::string::npos){ return; } // incorrect command format

    command = line.substr(0, space1_idx);
    dest_string = line.substr(space1_idx+1, space2_idx-space1_idx);
    message = line.substr(space2_idx + 1, std::string::npos);
    sscanf(dest_string.c_str(), "%d", &destination);

    std::cout << "Command: " << command << std::endl;
    std::cout << "Destination: " << destination << std::endl;
    std::cout << "Message: " << message << std::endl;
}

void close_server(int sig){
    s->close();
    end_session = true;
}

int main(int argc, char **argv) {
    std::cout << "Starting process with id " << argv[1] << std::endl;
    sscanf(argv[1], "%d", &process_id);

    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = close_server;
    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction");
        return 1;
    }

    parse_config();

    s = new Server(processes[process_id].port, processes.size());

    for(auto x: processes){
        unsigned int pid = x.first;
        struct connection info = x.second;
        info.client = new Client(info.ip, info.port, process_id);
    }

    while(!end_session){
        int fd = s->accept_client();
        if(fd >= 0){
            unprocessed_fds.push_back(fd);
        }
        process_fds();
        // process_input();
        // attempt_reconnections();
    }
}
