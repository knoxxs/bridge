#include "access.h"
#include "player_helper.h"
#include "psql_player.h"

void* playerMain(void*);
void connection_handler(int);
int getPlayerInfo(char *, char *, char *);

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
    logp(1,"PLAYER - New thread created succesfully");

    playerThreadArg playerInfo;
    playerInfo = *( (playerThreadArg*) (arg) );

    /////////////////////////____Getting player info____///////////////////////
    char id[9];
    char name[PLAYER_NAME_SIZE], team[PLAYER_TEAM_SIZE];
    int fd;

    strncpy(id, playerInfo.plid, 8);
    id[8] = '\0';

    fd = playerInfo.fd;

    if(getPlayerInfo(id , name, team) == 0 ){
        printf("This is it %s %s %s.\n", id, name, team);
    }else{
        //error
        exit(-1);
    }

    //////////////////////_____creating schedule alarm_____///////////////

    
}

int getPlayerInfo(char *plid, char *name, char *team){
    int ret;
    PGconn *conn = NULL;
    conn = ConnectDB("postgres","123321","bridge","127.0.0.1","5432");
    if (conn != NULL) {
        ret = getPlayerInfoFromDb(conn, plid, name, team);
        CloseConn(conn);
        return ret;// 0 - done, -1 - error in getPlayerInfo
    } else{
        //error
        printf("Error: establsihing connection tpo database \n");
        return -3;
    }
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
