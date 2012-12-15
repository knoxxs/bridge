#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>
#include <libpq-fe.h>
#include <string>
#include <iostream>
#include "access.h"
#include "psql_player.h"

#define PLAYER_NAME_SIZE 31 //30 + 1
#define PLAYER_TEAM_SIZE 9 //8+1
#define MAXLINE 2
#define UNIX_SOCKET_FILE "./demo_socket"
#define LOG_PATH "/home/abhi/Projects/bridge/server/log"

/* size of control buffer to send/recv one file descriptor */
#define CONTROLLEN  CMSG_LEN(sizeof(int))
 
static struct cmsghdr   *cmptr = NULL;      /* malloc'ed first time */


int unixSocket();
int recv_fd(int , ssize_t (*userfunc)(int, const void *, size_t), char*);
ssize_t errcheckfunc(int, const void *, size_t);
void* playerMain(void*);
void connection_handler(int);
int getPlayerInfo(char *, char *, char *);

struct playerThreadArg{
    int fd;
    char plid[8];
};

int main(){
	int socket_fd, connection_fd;
	struct sockaddr_un address;
	socklen_t address_length;
	
    int logfile;
    setLogFile(STDOUT_FILENO);
    logp("PLAYER-Main", 0,0 ,"Starting");
    if( (logfile = open(LOG_PATH, O_RDWR|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)) < 0 ){
        errorp("PLAYER-Main",0,0,"Error Opening logfile");
        debugp("PLAYER-Main",1,1,NULL);
    }
    setLogFile(logfile);

    logp("PLAYER-main",0,0,"Starting main");

    logp("PLAYER-main",0,0,"Calling unixSocket");
	socket_fd = unixSocket();
    logp("PLAYER-main",0,0,"Socket made Succesfully");

    logp("PLAYER-main",0,0,"Starting accepting connection in infinite while loop");
	while((connection_fd = accept(socket_fd, (struct sockaddr *) &address,&address_length)) > -1) {
		connection_handler(connection_fd);
		close(connection_fd);
	}
}


void* playerMain(void* arg){
    //logp(1,"PLAYER - New thread created succesfully");

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
int unixSocket(){
    struct sockaddr_un address;
    int socket_fd;
    socklen_t address_length;

    logp("PLAYER-unixSocket",0,0,"Inside unixSocket and calling socket");
    if( (socket_fd = socket(PF_UNIX, SOCK_STREAM, 0) ) < 0 ) {
        errorp("PLAYER-unixSocket",0,0,"Unable to create the socket");
        debugp("PLAYER-unixSocket",1,errno,NULL);
        return 1;
    } 

    unlink("./demo_socket");

    /* start with a clean address structure */
    memset(&address, 0, sizeof(struct sockaddr_un));

    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, sizeof(address.sun_path)-1, "./demo_socket");

    if(bind(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) != 0){
        printf("bind() failed\n");
        return 1;
    }

    if(listen(socket_fd, 5) != 0) {
        printf("listen() failed\n");
        return 1;
    }

    return socket_fd;
}


int recv_fd(int fd, ssize_t (*userfunc)(int, const void *, size_t), char *plid) {
    int             newfd, nr, status;
    char            *ptr;
    char            buf[MAXLINE];
    //struct iovec    iov[2];
    struct iovec    iov[1];
    struct msghdr   msg;

    status = -1;
    for ( ; ; ) 
    {
        iov[0].iov_base = buf;
        iov[0].iov_len  = sizeof(buf);
        //iov[1].iov_base = plid;
        //iov[1].iov_len  = 8;
        msg.msg_iov     = iov;
        //msg.msg_iovlen  = 2;
        msg.msg_iovlen  = 1;
        msg.msg_name    = NULL;
        msg.msg_namelen = 0;
        if (cmptr == NULL && (cmptr = malloc(CONTROLLEN)) == NULL)
            return(-1);
        msg.msg_control    = cmptr;
        msg.msg_controllen = CONTROLLEN;
        printf("aa %d aa\n",fd);
        nr = recvmsg(fd, &msg, 0);
        if (nr < 0) {
            printf("recvmsg errrrror %d %d %s\n",nr,errno,strerror(errno));
            //perror("recvmsg errrrror");
        } else if (nr == 0) {
            perror("connection closed by server");
            return(-1);
        }
        /*
        * See if this is the final data with null & status.  Null
        * is next to last byte of buffer; status byte is last byte.
        * Zero status means there is a file descriptor to receive.
        */
        for (ptr = buf; ptr < &buf[nr]; ) 
        {
            if (*ptr++ == 0) 
            {
                if (ptr != &buf[nr-1])
                    perror("message format error");
                status = *ptr & 0xFF;  /* prevent sign extension */
                if (status == 0) {
                    printf("msg_controllen(%d) and mine(%d)\n",msg.msg_controllen, CONTROLLEN );
                    if (msg.msg_controllen != CONTROLLEN)
                        perror("status = 0 but no fd");
                    newfd = *(int *)CMSG_DATA(cmptr);
                } else {
                    newfd = -status;
                }
                nr -= 2;
            }
        }
        if (nr > 0 && (*userfunc)(STDERR_FILENO, buf, nr) != nr)
            return(-1);
        if (status >= 0)    /* final data has arrived */
            return(newfd);  /* descriptor, or -status */
    }
}

ssize_t errcheckfunc(int a,const void *b, size_t c)
{
    return 0;
}
