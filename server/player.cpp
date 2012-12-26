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
#include "psql.h"

/* size of control buffer to send/recv one file descriptor */
#define CONTROLLEN  CMSG_LEN(sizeof(int))
#define PLAYER_NAME_SIZE 31 //30 + 1
#define PLAYER_TEAM_SIZE 9 //8+1
#define DATABASE_USER_NAME "postgres"
#define DATABASE_PASSWORD "123321"
#define DATABASE_NAME "bridge"
#define DATABASE_IP "127.0.0.1"
#define DATABASE_PORT "5432"
#define MAXLINE 2
#define UNIX_SOCKET_FILE "./demo_socket"
#define LOG_PATH "./log"
#define LISTEN_QUEUE_SIZE 5

static struct cmsghdr   *cmptr = NULL;      /* malloc'ed first time */

int unixSocket();
int recv_fd(int , ssize_t (*userfunc)(int, const void *, size_t), char*, int len);
ssize_t errcheckfunc(int, const void *, size_t);
void* playerMain(void*);
void connection_handler(int);
int getPlayerInfo(char *, char *, char *, int, int, int, int);


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

    //connecting to the database
    logp("PLAYER-main",0,0,"Connecting to the database");
    if (ConnectDB(DATABASE_USER_NAME, DATABASE_PASSWORD, DATABASE_NAME, DATABASE_IP, DATABASE_PORT, "DISPATCHER-main") == 0){
        logp("PLAYER-main",0,0,"Connected to database Successfully");
    }else {
        logp("PLAYER-main",0,0,"Unable to connect ot the database");
    }

    logp("PLAYER-main",0,0,"Starting accepting connection in infinite while loop");
	while(1){
        logp("PLAYER-main",0,0,"Waiting for the connection");        
        if( (connection_fd = accept(socket_fd, (struct sockaddr *) &address,&address_length)) > 0){
    		logp("PLAYER-main",0,0,"Connection recvd Succesfully and calling connection_handler");
            connection_handler(connection_fd);
    		close(connection_fd);
        }else{
            errorp("PLAYER-Main",0,0,"Error accepting connection");
            debugp("PLAYER-Main",1,1,NULL);
        }
	}

    //TODO graceful exit
    logp("PLAYER-main",0,0,"Closing the connection with the database");
    CloseConn("PLAYER-main");
}

int unixSocket(){
    struct sockaddr_un address;
    int socket_fd;
    socklen_t address_length;

    logp("PLAYER-unixSocket",0,0,"Inside unixSocket and calling socket");
    if( (socket_fd = socket(PF_UNIX, SOCK_STREAM, 0) ) < 0 ) {
        errorp("PLAYER-unixSocket",0,0,"Unable to create the socket");
        debugp("PLAYER-unixSocket",1,errno,NULL);
        return -1;
    } 


    unlink(UNIX_SOCKET_FILE);

    /* start with a clean address structure */
    memset(&address, 0, sizeof(struct sockaddr_un));

    logp("PLAYER-unixSocket",0,0,"Making struct");
    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, sizeof(address.sun_path)-1, "./demo_socket");

    logp("PLAYER-unixSocket",0,0,"Calling bind");
    if(bind(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) != 0){
        errorp("PLAYER-unixSocket",0,0,"Unable to bind to the socket");
        debugp("PLAYER-unixSocket",1,errno,NULL);
        return -1;
    }

    logp("PLAYER-unixSocket",0,0,"calling listen");
    if(listen(socket_fd,LISTEN_QUEUE_SIZE ) != 0) {
        errorp("PLAYER-unixSocket",0,0,"Unable to create the socket");
        debugp("PLAYER-unixSocket",1,errno,NULL);
        return -1;
    }

    logp("PLAYER-unixSocket",0,0,"returning socket fd");
    return socket_fd;
}

void connection_handler(int connection_fd){
    char msgbuf[50], plid[8], id[9];
    int fd_to_recv, ret, plid_len = sizeof(plid);

    char identity[40], buf[100];
    sprintf(identity, "PLAYER-connection_handler-fd: %d -", connection_fd);


    //recieving fd of the player
    logp(identity,0,0,"Calling recv_fd");
    fd_to_recv = recv_fd(connection_fd ,&errcheckfunc, plid, sizeof(plid));
    logp(identity,0,0,"fd recvd successfuly");

    //recieving plid of the player
    logp(identity,0,0,"recieving the plid fo the player");
    if((ret = recvall(connection_fd, plid, &plid_len, 0)) != 0) {
        errorp(identity,0,0,"Unable to recv the plid");
        debugp(identity,1,errno,NULL);
    }
    
    strcpy(id, plid);
    id[8] = '\0';
    
    sprintf(buf,"recvd fd is (%d) and plid is (%s)",fd_to_recv, id);
    logp(identity,0,0,buf);

    //checking if the fd we have recieved is correct or not
    logp(identity,0,0,"recieving the checking data");
    if((ret = recv(fd_to_recv, msgbuf, 6, 0)) == -1) {
        errorp(identity,0,0,"Unable to recv the checking data");
        debugp(identity,1,errno,NULL);
    }
    msgbuf[6] ='\0';
    sprintf(buf,"Checking data recvd is - %s", msgbuf);
    logp(identity,0,0,buf);
    
    //creating thread of the player
    pthread_t thread_id = 0;
    //void* thread_arg_v;
    int err;

    logp(identity,0,0,"Creating the thread argument");
    playerThreadArg thread_arg;//structure declared at top
    thread_arg.fd = fd_to_recv;
    strcpy(thread_arg.plid, plid);
    //thread_arg_v = (void*) thread_arg;//this is not possible may be because size of struct is more than size of void pointer

    logp(identity,0,0,"Calling Calling pthread_create");
    if((err = pthread_create(&thread_id, NULL, playerMain, &thread_arg ))!=0) {
        errorp(identity,0,0,"Unable to create the thread");
        debugp(identity,1,err,NULL);
    }

    logp(identity,0,0,"Calling pthread_detach");
    if ((err = pthread_detach(thread_id)) != 0) {
        errorp(identity,0,0,"Unable to detach the thread");
        debugp(identity,1,err,NULL);
    }

    logp(identity,0,0,"Closing fd_to_recv");
    close(fd_to_recv);

    return;
}

void* playerMain(void* arg){
    playerThreadArg playerInfo;
    playerInfo = *( (playerThreadArg*) (arg) );

    char id[9];
    char name[PLAYER_NAME_SIZE], team[PLAYER_TEAM_SIZE];
    int fd;

    strncpy(id, playerInfo.plid, 8);
    id[8] = '\0';
    fd = playerInfo.fd;

    char identity[40], buf[100];
    sprintf(identity, "PLAYER-playerMain-fd: %d -", fd);

    logp(identity,0,0,"New thread created Succesfully");

    logp(identity,0,0,"Calling get player info");
    if(getPlayerInfo(id , name, team, fd, sizeof(id), sizeof(name), sizeoof(team)) == 0 ){
        sprintf(buf,"This is player info id(%s) name(%s) team(%s)\n", id, name, team);
        logp(identity,0,0,buf);
    }else{
        logp(identity,0,0,"Unable to retrieve the player info, exiting from this thread");
        exit(-1);
    }

    //////////////////////_____creating schedule alarm_____///////////////
}

int getPlayerInfo(char *plid, char *name, char *team, int identity_fd,int p_len, int n_len, int t_len){
    int ret;
    char identity[40], buf[100];
    sprintf(identity, "PLAYER-getPlayerInfo-fd: %d -", identity_fd);
    
    logp(identity,0,0,"Connected to data Succesfully and calling getPlayerInfoFromDb");
    ret = getPlayerInfoFromDb(plid, name, team, identity);
    logp(identity,0,0,"Returned from getPlayerInfoFromDb");

    sprintf(buf,"Returning from getPlayerInfo with ret value (%d)",ret);
    logp(identity,0,0,buf);
    return ret;// 0 - done, -1 - error in getPlayerInfoFromDb
}

int recv_fd(int fd, ssize_t (*userfunc)(int, const void *, size_t), char *plid, int len) {
    int             newfd, nr, status;
    char            *ptr;
    char            buf[MAXLINE];
    //struct iovec    iov[2];
    struct iovec    iov[1];
    struct msghdr   msg;

    char identity[40], tempbuf[100];
    sprintf(identity, "PLAYER-recv_fd-fd: %d -", fd);

    logp(identity,0,0,"Entering the infite for loop");
    status = -1;
    for ( ; ; ) 
    {
        logp(identity,0,0,"Stuffing iovec");
        iov[0].iov_base = buf;
        iov[0].iov_len  = sizeof(buf);
        //iov[1].iov_base = plid;
        //iov[1].iov_len  = 8;

        logp(identity,0,0,"Filling msghdr");
        msg.msg_iov     = iov;
        //msg.msg_iovlen  = 2;
        msg.msg_iovlen  = 1;
        msg.msg_name    = NULL;
        msg.msg_namelen = 0;

        logp(identity,0,0,"Allocating memory");
        if (cmptr == NULL && (cmptr = malloc(CONTROLLEN)) == NULL)
            return(-1);

        logp(identity,0,0,"Assinging allocated memory to msghdr");
        msg.msg_control    = cmptr;

        sprintf(tempbuf,"Adding lenght of allocated memory, lenght - controllen(%d)",CONTROLLEN);
        logp(identity,0,0,tempbuf);
        msg.msg_controllen = CONTROLLEN;
        
        logp(identity,0,0,"Calling recvmsg");
        nr = recvmsg(fd, &msg, 0);
        if (nr < 0) {
            errorp(identity,0,0,"Unable to recv the msg");
            debugp(identity,1,errno,NULL);
            return -1;
        } else if (nr == 0) {
            logp(identity,0,0,"Unable to recv the msg becz connection is closed by client");
            return -1;
        }
        /*
        * See if this is the final data with null & status.  Null
        * is next to last byte of buffer; status byte is last byte.
        * Zero status means there is a file descriptor to receive.
        */
        logp(identity,0,0,"Entering the data traversing for loop");
        for (ptr = buf; ptr < &buf[nr]; ) 
        {
            logp(identity,0,0,"Inside traversing for loop");
            if (*ptr++ == 0) 
            {
                logp(identity,0,0,"Recvd First zero");
                if (ptr != &buf[nr-1])
                    errorp(identity,0,0,"Message Format error");
                status = *ptr & 0xFF;  /* prevent sign extension */
                if (status == 0) {
                    sprintf(tempbuf,"msg_controllen recvd is(%d) and must be is(%d)\n",msg.msg_controllen, CONTROLLEN );
                    logp(identity,0,0,tempbuf);
                    if (msg.msg_controllen != CONTROLLEN)
                        logp(identity,0,0,"Status =0 but no fd");
                    newfd = *(int *)CMSG_DATA(cmptr);
                    sprintf(tempbuf, "New fd extracted is %d",newfd);
                    logp(identity,0,0,tempbuf);
                } else {
                    logp(identity,0,0,"assigning newfd = error status");
                    newfd = -status;
                }
                nr -= 2;
            }
        }

        logp(identity,0,0,"Checking whether fd recvd succesfully or not");
        if (nr > 0 && (*userfunc)(STDERR_FILENO, buf, nr) != nr)
            logp(identity,0,0,"Error extracting error from the msg");
            //return(-1);
        if (status >= 0){    /* final data has arrived */
            logp(identity,0,0,"Returning new fd");
            return(newfd);  /* descriptor, or -status */
        }
    }
}

ssize_t errcheckfunc(int a,const void *b, size_t c){
    logp("PLAYER-errcheckfunc",0,0,"Inside error check function");
    return 0;
}
