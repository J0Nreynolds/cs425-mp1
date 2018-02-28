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
    int event_idx;
};

std::unordered_map<unsigned int, struct connection> processes;
std::unordered_map<unsigned int, struct fd_information> fd_info;

unsigned int process_id;
static Server* s;
static bool end_session = false;
static int kq;
static int event_idx = 0;
static struct kevent* chlist;
static struct kevent* evlist;

/**
 * Initializes the unordered_map holding the information needed to connect to each process.
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

void print_kevents(){
    std::cout << event_idx << " events: ";
    for(int i = 0; i < event_idx; i++){
        std::cout << chlist[i].ident << " ";
    }
    std::cout << std::endl;
}

/**
 * Removes kevent listener for file descriptor
 */
void remove_fd_from_kevent(int fd){
    int idx = fd_info[fd].event_idx;
    fd_info.erase(fd);

    event_idx -= 1;
    unsigned int temp_fd = chlist[event_idx].ident;
    if(temp_fd != fd){ // We want to swap current event for last event, unless the current event is last
        // Deleting last event
        EV_SET(&chlist[event_idx], temp_fd, EVFILT_READ, EV_DELETE, 0, 0, 0);
        // Reenable last event's fd in event we just deleted
        EV_SET(&chlist[idx], temp_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
    }
    // else event we want to remove is at end
    // -> do nothing, we already decremented the event_idx to exclude it from kevent call
}

/**
 * Processes any updated file descriptors
 */
void process_fds(){
    struct timespec tmout = { 0, 0 };  /* return immediately if no new events */
    int nev = kevent(kq, chlist, event_idx, evlist, event_idx, &tmout);
    if (nev == -1) {
       perror("kevent()");
       exit(1);
    }
    for(int i = 0; i < nev; i ++){
        int fd = evlist[i].ident;
        if (evlist[i].flags & EV_ERROR) {                /* report errors */
           fprintf(stderr, "EV_ERROR: %s\n%d\n", strerror(evlist[i].data), fd);
           exit(1);
        };
        if(fd_info[fd].server_owned){ // Then it's a remote client telling us their ID so we can connect to them
            // Read the process id sent from the client
        	if(evlist[i].flags & EV_EOF){ // Handle client disconnect
                std::cout << "Socket fd " << fd << " for process " << fd_info[fd].pid << " disconnected." << std::endl;
                ::close(fd);
                remove_fd_from_kevent(fd);
                return;
            }
            unsigned int pid = 0;
            int read_bytes = read_all_from_socket(fd, (char *) &pid, sizeof(int));
            if(read_bytes == -1){ // Error case
                std::cout << "Error reading from socket" <<read_bytes<<std::endl;
                exit(1);
        	}
            std::cout << "Got id: " << pid << " from client with fd: " << fd << std::endl;
            fd_info[fd].pid = pid;
            if(pid != process_id && !processes[pid].client->is_connected()){ // If we don't already have a client for pid
                struct connection info = processes[pid];
                std::cout << "Attempting to connect back to server " << pid << " at port " << info.port << std::endl;
                int client_fd = info.client->connect_to_server(info.ip, info.port);
                if(client_fd >= 0){ // Success
                    fd_information f_info;
                    f_info.pid = pid;
                    f_info.server_owned = false;
                    f_info.event_idx = event_idx;
                    std::pair<unsigned int, struct fd_information> entry(client_fd, f_info);
                    fd_info.insert(entry);

                    /* Initialize kevent structure. */
                    EV_SET(&chlist[event_idx++], client_fd, EVFILT_READ, EV_ADD, 0, 0, 0);
                }
            }
        }
        else { //Then there is an update from another server. TODO: Process the update.

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
        std::cin.clear();
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
    delete[] chlist;
    delete[] evlist;
    for(auto x: fd_info){
        int fd = x.first;
        struct fd_information info = x.second;
        if(info.server_owned){
            shutdown(fd, SHUT_RDWR);
            ::close(fd);
            remove_fd_from_kevent(fd);
        }
        else {
            ::close(fd);
        }
    }
    s->close();
    delete s;
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

    chlist = new struct kevent[2*processes.size()]; // 2 fds per process
    evlist = new struct kevent[2*processes.size()]; // 2 fds per process

    if ((kq = kqueue()) == -1) {
       perror("kqueue");
       exit(1);
    }

    s = new Server(processes[process_id].port, processes.size());

    for(auto x: processes){
        unsigned int pid = x.first;
        processes[pid].client = new Client(processes[pid].ip, processes[pid].port, process_id);
        if(processes[pid].client->is_connected()){
            fd_information f_info;
            f_info.pid = pid;
            f_info.server_owned = false;
            f_info.event_idx = event_idx;
            int client_fd = processes[pid].client->get_socket_fd();
            std::pair<unsigned int, struct fd_information> entry(client_fd, f_info);
            fd_info.insert(entry);

            /* Initialize kevent structure. */
            EV_SET(&chlist[event_idx++], client_fd, EVFILT_READ, EV_ADD, 0, 0, 0);
        }
    }

    // Set cin to be non-blocking
    int flags = fcntl(0, F_GETFL, 0);
    fcntl(0, F_SETFL, flags | O_NONBLOCK);

    while(!end_session){
        int fd = s->accept_client();
        if(fd >= 0){
            fd_information info;
            info.pid = -1;
            info.server_owned = true;
            info.event_idx = event_idx;
            std::pair<unsigned int, struct fd_information> entry(fd, info);
            fd_info.insert(entry);

            /* Initialize kevent structure. */
            EV_SET(&chlist[event_idx++], fd, EVFILT_READ, EV_ADD, 0, 0, 0);
        }
        process_fds();
        process_input();
        // attempt_reconnections();
    }
}
