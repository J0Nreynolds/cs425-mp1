#include <iostream>
#include <list>
#include <thread>
#include <mutex>

#include <signal.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#include "server.h"
#include "client.h"
#include "common.h"

using namespace std;

struct connection {   // Declare connection struct type
    std::string ip;
    std::string port;
    Client* client;
};

struct fd_information {
    unsigned int pid;
    bool server_owned;
};

std::unordered_map<unsigned int, struct connection> processes;
std::list<unsigned int> unprocessed_fds;
std::unordered_map<unsigned int, struct fd_information> fd_info;

unsigned int process_id;
static Server* s;
static bool end_session = false;
static int kq;
static int event_idx = 0;
static struct kevent* chlist;
static struct kevent* evlist;

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
 * Processes any updated file descriptors
 */
void process_fds(){
    struct timespec tmout = { 0, 0 };
    int nev = kevent(kq, chlist, event_idx, evlist, event_idx, &tmout);   /* return immediately if no new events */
    if (nev == -1) {
       perror("kevent()");
       exit(1);
    }
    for(int i = 0; i < nev; i ++){
        int fd = evlist[i].ident;
        if (evlist[i].flags & EV_ERROR) {                /* report errors */
           fprintf(stderr, "EV_ERROR: %s\n", strerror(evlist[i].data));
           exit(1);
        };
        unsigned int pid = 0;
        int read_bytes = read_all_from_socket(fd, (char *) &pid, sizeof(int));
    	if(read_bytes != sizeof(int)){
            unprocessed_fds.push_back(fd); // Add back for an attempt later.
    		// std::cout << "Error: " << read_bytes << " bytes read instead of " << sizeof(int) << std::endl;
    	}
        else {
            std::cout << "Got id: " << pid << " from client with fd: " << fd << std::endl;
            if(fd_info[fd].server_owned){
                fd_info[fd].pid = pid;
                if(pid != process_id && !processes[pid].client->is_connected()){
                    struct connection info = processes[pid];
                    std::cout << "Attempting to connect back to server " << pid << " at port " << info.port << std::endl;
                    int client_fd = info.client->connect_to_server(info.ip, info.port);
                    if(client_fd >= 0){
                        fd_information f_info;
                        f_info.pid = pid;
                        f_info.server_owned = false;
                        std::pair<unsigned int, struct fd_information> entry(client_fd, f_info);
                        fd_info.insert(entry);

                        /* Initialize kevent structure. */
                        EV_SET(&chlist[event_idx++], client_fd, EVFILT_READ, EV_ADD, 0, 0, 0);
                    }
                }
            }
            else {
                //Then there is an update from another server. Process.
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

    chlist = new struct kevent[processes.size()];
    evlist = new struct kevent[processes.size()];

    if ((kq = kqueue()) == -1) {
       perror("kqueue");
       exit(1);
    }

    s = new Server(processes[process_id].port, processes.size());

    for(auto x: processes){
        unsigned int pid = x.first;
        processes[pid].client = new Client(processes[pid].ip, processes[pid].port, process_id);
        if(processes[pid].client->is_connected()){

        }
    }

    while(!end_session){
        int fd = s->accept_client();
        if(fd >= 0){
            fd_information info;
            info.pid = -1;
            info.server_owned = true;
            std::pair<unsigned int, struct fd_information> entry(fd, info);
            fd_info.insert(entry);

            /* Initialize kevent structure. */
            EV_SET(&chlist[event_idx++], fd, EVFILT_READ, EV_ADD, 0, 0, 0);
        }
        process_fds();
        // process_input();
        // attempt_reconnections();
    }
}
