#include "access.h"
#include "player_helper.h"

void* playerMain(void*);
void connection_handler(int);
struct playerThreadArg{
    int fd;
    char plid[8];
};

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

void* playerMain(void* arg){
    playerThreadArg playerInfo;
    playerInfo = *( (playerThreadArg*) (arg) );

	logp(1,"PLAYER - New thread created succesfully");

}

void connection_handler(int connection_fd){
	int fd_to_recv, recvdBytes;
	char msgbuf[50], plid[8], id[9];
	
	//recieving fd of the player
	fd_to_recv = recv_fd(connection_fd ,&errcheckfunc, plid);
	printf("message received:%d\n",fd_to_recv);
	
	//recieving plid of hte player
	if((recvdBytes = recv(connection_fd, plid, 8, 0)) == -1) {
        fprintf(stderr, "Error receiving data %d\n", errno);
    }	
    strcpy(id, plid);
    id[8] = '\0';
    printf("plid is:%s\n",id);

    //checking is the fd we have recieved is correct or not
    if((recvdBytes = recv(fd_to_recv, msgbuf, 6, 0)) == -1) {
        fprintf(stderr, "Error receiving data %d\n", errno);
    }
    msgbuf[6] ='\0';
    printf("message received:%s\n",msgbuf);
    
    
    //creating thread of the player
    pthread_t thread_id = 0;
    //void* thread_arg_v;
    int err;
    playerThreadArg thread_arg;//structure declared at top
    thread_arg.fd = fd_to_recv;
    strcpy(thread_arg.plid, plid);
    //thread_arg_v = (void*) thread_arg;//this is not possible may be because size of struct is more than size of void pointer

    if((err = pthread_create(&thread_id, NULL, playerMain, &thread_arg ))!=0) {
        errorp("PLAYER-pthrad_create:", 1, err, NULL);
    }
    if ((err = pthread_detach(thread_id)) != 0) {
        errorp("PLAYER-pthread_detach:", 1, err, NULL);
    }

    close(fd_to_recv);

    return;
}
