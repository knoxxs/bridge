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
#define UNIX_SOCKET_FILE_DIS_TO_PLA "./demo_socket"
#define UNIX_SOCKET_FILE_PLA_TO_GAM "./UNIX_SOCKET_FILE_PLA_TO_GAM"
#define LOG_PATH "./log"
#define LISTEN_QUEUE_SIZE 5
#define IDENTITY_SIZE 40

static struct cmsghdr   *cmptr = NULL;      /* malloc'ed first time */

int unixSocket();
int recv_fd(int , ssize_t (*userfunc)(int, const void *, size_t), char*, int len);
ssize_t errcheckfunc(int, const void *, size_t);
void* playerMain(void*);
void connection_handler(int);
int getPlayerInfo(char *, char *, char *, int, int, int, int);
void contactGame(char*,int,int len);
int send_fd(int, int, char*, int len);
int unixClientSocket(char*, int len);
int send_err(int, int, const char *, int len);


struct playerThreadArg{
    int fd;
    char plid[9];
};

int main(){
	int socket_fd, connection_fd;
	struct sockaddr_un address;
	socklen_t address_length = sizeof(address);
	
    int logfile;
    setLogFile(STDOUT_FILENO);
    logp("PLAYER-Main", 0,0 ,"Starting");
    if( (logfile = open(LOG_PATH, O_RDWR|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)) < 0 ){
        errorp("PLAYER-Main",0,0,"Error Opening logfile");
        debugp("PLAYER-Main",1,1,"");
    }
    setLogFile(logfile);

    logp("PLAYER-main",0,0,"Starting main");

    logp("PLAYER-main",0,0,"Calling unixSocket");
	socket_fd = unixSocket();
    logp("PLAYER-main",0,0,"Socket made Succesfully");

    //connecting to the database
    logp("PLAYER-main",0,0,"Connecting to the database");
    if (ConnectDB(DATABASE_USER_NAME, DATABASE_PASSWORD, DATABASE_NAME, DATABASE_IP, DATABASE_PORT, "PLAYER-main") == 0){
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
            debugp("PLAYER-Main",1,errno,"");
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
    char buf[100];

    logp("PLAYER-unixSocket",0,0,"Inside unixSocket and calling socket");
    if( (socket_fd = socket(PF_UNIX, SOCK_STREAM, 0) ) < 0 ) {
        errorp("PLAYER-unixSocket",0,0,"Unable to create the socket");
        debugp("PLAYER-unixSocket",1,errno,"");
        return -1;
    } 


    unlink(UNIX_SOCKET_FILE_DIS_TO_PLA);

    /* start with a clean address structure */
    memset(&address, 0, sizeof(struct sockaddr_un));

    logp("PLAYER-unixSocket",0,0,"Making struct");
    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, sizeof(address.sun_path)-1, UNIX_SOCKET_FILE_DIS_TO_PLA);

    logp("PLAYER-unixSocket",0,0,"Calling bind");
    if(bind(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) != 0){
        errorp("PLAYER-unixSocket",0,0,"Unable to bind to the socket");
        debugp("PLAYER-unixSocket",1,errno,"");
        return -1;
    }

    logp("PLAYER-unixSocket",0,0,"calling listen");
    if(listen(socket_fd,LISTEN_QUEUE_SIZE ) != 0) {
        errorp("PLAYER-unixSocket",0,0,"Unable to create the socket");
        debugp("PLAYER-unixSocket",1,errno,"");
        return -1;
    }

    sprintf(buf,"returning socket_fd %d",socket_fd);
    logp("PLAYER-unixSocket",0,0,buf);
    return socket_fd;
}

void connection_handler(int connection_fd){
    char msgbuf[50], plid[9];
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
        debugp(identity,1,errno,"");
    }
    
    sprintf(buf,"recvd fd is (%d) and plid is (%s)",fd_to_recv, plid);
    logp(identity,0,0,buf);

    //checking if the fd we have recieved is correct or not
    logp(identity,0,0,"recieving the checking data");
    if((ret = recv(fd_to_recv, msgbuf, 6, 0)) == -1) {
        errorp(identity,0,0,"Unable to recv the checking data");
        debugp(identity,1,errno,"");
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
        debugp(identity,1,err,"");
    }

    logp(identity,0,0,"Calling pthread_detach");
    if ((err = pthread_detach(thread_id)) != 0) {
        errorp(identity,0,0,"Unable to detach the thread");
        debugp(identity,1,err,"");
    }

    logp(identity,0,0,"Closing fd_to_recv");
    close(fd_to_recv);

    return;
}

void* playerMain(void* arg){
    playerThreadArg playerInfo;
    playerInfo = *( (playerThreadArg*) (arg) );

    char plid[9];
    char name[PLAYER_NAME_SIZE], team[PLAYER_TEAM_SIZE];
    int fd;

    strncpy(plid, playerInfo.plid, 9);
    fd = playerInfo.fd;

    char identity[40], buf[100];
    sprintf(identity, "PLAYER-playerMain-fd: %d -", fd);

    logp(identity,0,0,"New thread created Succesfully");

    sprintf(buf, "Calling get player info with plid %s", plid);
    logp(identity,0,0,buf);
    if(getPlayerInfo(plid , name, team, fd, sizeof(plid), sizeof(name), sizeof(team)) == 0 ){
        sprintf(buf,"This is player info id(%s) name(%s) team(%s)\n", plid, name, team);
        logp(identity,0,0,buf);
    }else{
        logp(identity,0,0,"Unable to retrieve the player info, exiting from this thread");
        exit(-1);
    }

    //////////////////////_____creating schedule alarm_____///////////////
    
    //check for latest match
    string timestamp;
    if( getPlayerSchedule(plid , timestamp, identity) == 0){
        sprintf(buf,"This is player next match timestamp (%s)\n", timestamp.c_str());
        logp(identity,0,0,buf);
    }else{
        logp(identity,0,0,"Unable to retrieve next match timestamp, exiting from this thread");
        exit(-1);
    }
    
    //create a alarm which checks out time remaining.....
    // Sleep for 1.5 sec
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 500000;
    //select(0, NULL, NULL, NULL, &tv);
    //create the game and send the data

    sprintf(buf,"Player id is %s and Contacting Game",plid);
    logp(identity,0,0,buf);
    contactGame( plid, fd, sizeof(plid));
    logp(identity,0,0,"Contacted To Game succesfully");


}

int getPlayerInfo(char *plid, char *name, char *team, int identity_fd,int p_len, int n_len, int t_len){
    int ret;
    char identity[40], buf[100];
    sprintf(identity, "PLAYER-getPlayerInfo-fd: %d -", identity_fd);
    
    sprintf(buf, "Connected to data Succesfully and calling getPlayerInfoFromDb with plid %s",plid);
    logp(identity,0,0,buf);
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
            debugp(identity,1,errno,"");
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

void contactGame(char* plid, int fd_to_send, int len){
    int socket_fd;

    char identity[IDENTITY_SIZE], buf[100];
    sprintf(identity, "PLAYER-contact-Player-fd: %d -", fd_to_send);    

    logp(identity,0,0,"Inside contactPlayer and calling unixClientSocket");
    sprintf(buf,"PLAYER-unixClientSocket-fd: %d -",fd_to_send);
    socket_fd = unixClientSocket(buf,sizeof(buf));
    sprintf(buf,"Recved socket-fd: %d -",socket_fd);
    logp(identity,0,0,buf);

    logp(identity,0,0,"Calling send_fd");
    send_fd(socket_fd, fd_to_send, plid, len);
    logp(identity,0,0,"fd sent Successfully");

    sprintf(buf, "Sending plid to game process ,plid_length(%d)",len);
    logp(identity,0,0,buf);
    if( sendall(socket_fd, plid, &len, 0) != 0){
        errorp(identity, 0, 0, "Unable to send complete plid");
        debugp(identity,1,errno,"");
    }
}

int unixClientSocket(char* identity, int len){
    struct sockaddr_un address;
    int  socket_fd;

    logp(identity,0,0,"Calling socket");
    if( (socket_fd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
        errorp(identity,0,0,"Unable to make the socket");
        debugp(identity,1,errno,"");
        return -1;
    }

    /* start with a clean address structure */
    logp(identity,0,0,"Cleaning the struct");
    memset(&address, 0, sizeof(struct sockaddr_un));

    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, sizeof(address.sun_path)-1, UNIX_SOCKET_FILE_PLA_TO_GAM);

    logp(identity,0,0,"Calling Connect");
    if(connect(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) != 0) {
        errorp(identity,0,0,"Unable to connect the socket");
        debugp(identity,1,errno,"");
        return -1;
    }

    logp(identity,0,0,"Returning socket_fd");
    return socket_fd;
}

int send_err(int fd, int errcode, const char *msg, int len){
    int n;

    if ((n = strlen(msg)) > 0)
        if (write(fd, msg, n) != n)    /* send the error message */
            return(-1);

    if (errcode >= 0)
        errcode = -1;   /* must be negative */

    if (send_fd(fd, errcode, NULL,0) < 0) //NULL for plid
        return(-1);

    return(0);
}

int send_fd(int fd, int fd_to_send, char* plid, int len){
    ssize_t temp;
    //struct iovec    iov[2];//second is for sending plid
    struct iovec    iov[1];//second is for sneding plid
    struct msghdr   msg;
    char            buf[2]; /* send_fd()/recv_fd() 2-byte protocol */

    char identity[IDENTITY_SIZE], tempbuf[100];
    sprintf(identity, "PLAYER-send_fd-fd: %d -", fd_to_send);    

    logp(identity,0,0,"Adding bufs to iovec");
    iov[0].iov_base = buf;
    iov[0].iov_len  = 2;
    // iov[1].iov_base = plid;
    // iov[1].iov_len  = 8;

    logp(identity,0,0,"Adding iovec to msghdr");
    msg.msg_iov     = iov;
    // msg.msg_iovlen  = 2;
    msg.msg_iovlen  = 1;
    msg.msg_name    = NULL;
    msg.msg_namelen = 0;


    if (fd_to_send < 0) {
        logp(identity,0,0,"When fd_to_send is < 0, defining the structure of buf according to the protocol");
        msg.msg_control    = NULL;
        msg.msg_controllen = 0;
        buf[1] = -fd_to_send;   /* nonzero status means error */
        if (buf[1] == 0)
            buf[1] = 1; /* -256, etc. would screw up protocol */
    } else {
        logp(identity,0,0,"When fd_to_send > 0, assigning memory");
        if (cmptr == NULL && (cmptr = malloc(CONTROLLEN)) == NULL)
            return(-1);
        logp(identity,0,0,"Giving rights");
        cmptr->cmsg_level  = SOL_SOCKET;
        cmptr->cmsg_type   = SCM_RIGHTS;
        cmptr->cmsg_len    = CONTROLLEN;
        msg.msg_control    = cmptr;
        msg.msg_controllen = CONTROLLEN;

        sprintf(tempbuf,"Adding data with controllen(%d)",CONTROLLEN);
        logp(identity,0,0,tempbuf);
        *(int *)CMSG_DATA(cmptr) = fd_to_send;     /* the fd to pass */
        buf[1] = 0;          /* zero status means OK */
    }


    buf[0] = 0;              /* null byte flag to recv_fd() */
    logp(identity,0,0,"Sending the fd and the msg");
    temp = sendmsg(fd, &msg, 0);
    if (temp != 2){
        if(temp == -1){
            errorp(identity,0,0,"Unable to send the data");
            debugp(identity,1,errno,NULL);
        }else {
            errorp(identity,0,0,"Unable to send complete data");
        }

        logp(identity,0,0,"Returning Unsuccessful");
        return(-1);
    }

    logp(identity,0,0,"Returning Successfully");
    return(0);
}
