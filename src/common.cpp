#include "common.h"

int read_all_from_socket(int socket, char *buffer, size_t count) {
	int total_bytes = 0;
	while(count){
		int read_bytes = read(socket, buffer, count);
		if(read_bytes == -1 && errno != EINTR){
			return -1;
		}
		if(read_bytes == 0) return total_bytes;
		if(read_bytes > 0){
			count -= read_bytes;
			buffer += read_bytes;
			total_bytes += read_bytes;
		}
	}
	return total_bytes;
}

int write_all_to_socket(int socket, const char *buffer, size_t count) {
	int total_bytes = 0;
	while(count){
		int written_bytes = write(socket, buffer, count);
		if(written_bytes == -1 && errno != EINTR) {
			return -1;
		}
		if(written_bytes == 0) return total_bytes;
		if(written_bytes > 0) {
			count -=  written_bytes;
			buffer += written_bytes;
			total_bytes += written_bytes;
		}

	}
	return total_bytes;
}
