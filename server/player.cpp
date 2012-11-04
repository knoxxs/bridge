#include "access.h"
#include "player_helper.h"

void signalHandler();

int main(){

	// static struct sigaction sigstruct;
	// sigfillset(&(sigstruct.sa_mask));//add all the signals
	// sigstruct.sa_handler = signalHandler;
	// sigstruct.sa_flags |= SA_RESTART;

	int socket_fd, connection_fd;
	struct sockaddr_un address;
	socklen_t address_length;
	
	socket_fd = unixSocket();

	while((connection_fd = accept(socket_fd, (struct sockaddr *) &address,&address_length)) > -1) {
		connection_handler(connection_fd);
		close(connection_fd);
	}
}

void signalHandler(){

}