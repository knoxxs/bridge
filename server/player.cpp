#include "access.h"
#include "player_helper.h"

void* playerMain(void*);
void connection_handler(int);

int main(){

	// static struct sigaction sigstruct;
	// sigfillset(&(sigstruct.sa_mask));//add all the signals
	// sigstruct.sa_handler = playerMain;
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

void* playerMain(void* lp){
    int fd = (long)lp;	
	logp(1,"PLAYER - New thread created succesfully");

}

void connection_handler(int connection_fd){
	int fd_to_recv, recvdBytes;
	char msgbuf[50];
	fd_to_recv = recv_fd(connection_fd ,&errcheckfunc);
	
    printf("message received:%d\n",fd_to_recv);
	
    // if((recvdBytes = recv(fd_to_recv, msgbuf, 6, 0)) == -1) {
    //     fprintf(stderr, "Error receiving data %d\n", errno);
    // }
    // msgbuf[6] ='\0';
    // printf("message received:%s\n",msgbuf);

    pthread_t thread_id = 0;
    void *thread_arg;
    int err;

    thread_arg=(void *)fd_to_recv;
    if((err = pthread_create(&thread_id, NULL, playerMain,thread_arg ))!=0) {
        errorp("PLAYER-pthrad_create:", 1, err, NULL);
    }
    if ((err = pthread_detach(thread_id)) != 0) {
        errorp("PLAYER-pthread_detach:", 1, err, NULL);
    }

}
