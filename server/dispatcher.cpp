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
#include "myregex.h"

#define PLAYER_PROCESS_NAME "player"
#define LISTEN_QUEUE_SIZE 10
#define CONTROLLEN  CMSG_LEN(sizeof(int))


void *get_in_addr(struct sockaddr*);
int makeSocketForClients();
int socketBind(struct addrinfo *ai);
void* SocketHandler(void* lp);
int login(int fd, char* plid);
void contactPlayer(char*,int);
int send_fd(int, int, char*);
int unixClientSocket();
int send_err(int, int, const char *);

static struct cmsghdr   *cmptr = NULL;  /* malloc'ed first time */ 
int playerProcId;

int main(int argv, char** argc) {

    logp("DISPATCHER-main",0,0,"");
    logp("DISPATCHER-main",0,0,"Starting main");

    int listener; // listening the socket descriptor
    int newfd; //newly accept()ed file descriptor
    struct sockaddr_storage clientaddr; // client address
    socklen_t addrlen;
    char buf[256]; //buffer for client
    char remoteIP[INET6_ADDRSTRLEN];
    
    pthread_t thread_id = 0;
    void *thread_arg;
    void **thread_ret;

    int err;

    logp("DISPATCHER-main",0,0,"Calling makeSocketForClients");
    listener = makeSocketForClients();
    logp("DISPATCHER-main",0,0,"Successfully made socket for client");
    
    //Finding pid of player process
    logp("DISPATCHER-main",0,0,"Calling the processIdFinder to find the id of the player process");
    playerProcId = processIdFinder(PLAYER_PROCESS_NAME);
    logp("DISPATCHER-main",0,0,"Successfully Found the player process ID");
    debugp("DISPATCHER-main",0,0,snprintf(buf, "The ID - %d", playerProcId));
    
    logp("DISPATCHER-main",0,0,"Enetering the everlistening while loop");
    while (1) { 
        logp("DISPATCHER-main",0,0,"Waiting for the connection");
        
        logp("DISPATCHER-main",0,0,"Calling accept");
        if((newfd = accept(listener, (struct sockaddr *)&clientaddr, &addrlen)) == -1){
            errorp("DISPATCHER-main",0,0,"unable to accept the connection");
            debugp("DISPATCHER-main",1,errno,NULL);
            continue;
        }
        logp("DISPATCHER-main",0,0,"Accepted New Connection");
        
        logp("DISPATCHER-main",0,0,"Calling inet_ntop");
        inet_ntop(clientaddr.ss_family, get_in_addr((struct sockaddr *)&clientaddr), remoteIP, INET6_ADDRSTRLEN);
        logp("DISPATCHER-main",0,0,sprintf(buf,"Got new connection from IP - %s", remoteIP));
    
        logp("DISPATCHER-main",0,0,"Making newfd as thread argument compatible");
        thread_arg=(void *)newfd;

        logp("DISPATCHER-main",0,0,"Calling pthread");
        if((err = pthread_create(&thread_id, NULL, SocketHandler,thread_arg ))!=0) {
            errorp("DISPATCHER-main",0,0,"Unable to create the thread");
            debugp("DISPATCHER-main",1,err,NULL);
        }
        
        logp("DISPATCHER-main",0,0,"Calling pthread_detach");
        if ((err = pthread_detach(thread_id)) != 0){
            errorp("DISPATCHER-main",0,0,"Unable to detach the thread");
            debugp("DISPATCHER-main",1,err,NULL);
        }
    }

    logp("DISPATCHER-main",0,0,"after the infinite while loop - THIS IS IMPOSSIBLE");
    return -1;
}

int socketBind(struct addrinfo *ai){
    struct addrinfo  *p;
    int yes = 1, ret;
    int listener;

    logp("DISPATCHER-socketBind",0,0,"Starting the main for loop to see supported structs");
    for (p = ai; p != NULL; p = p->ai_next) {
        logp("DISPATCHER-socketBind",0,0,"Calling socket");
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) {
            errorp("DISPATCHER-socketBind",0,0,"Unable to make the socket");
            debugp("DISPATCHER-socketBind",1,errno,NULL);
            continue;
        }
        logp("DISPATCHER-socketBind",0,0,"Socket formed succesfully");
        
        logp("DISPATCHER-socketBind",0,0,"Calling setsockopt");
        if ((setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int)))) {
            errorp("DISPATCHER-socketBind",0,0,"Unable to manipulate the socket");
            debugp("DISPATCHER-socketBind",1,errno, NULL);
            exit(1);
        }
        logp("DISPATCHER-socketBind",0,0,"setsockopt done succesfully");
        
        logp("DISPATCHER-socketBind",0,0,"Calling bind");
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            errorp("DISPATCHER-socketBind",0,0,"Unable to bind the socket");
            debugp("DISPATCHER-socketBind",1,errno,NULL);
            close(listener);
            continue;
        }
        logp("DISPATCHER-socketBind",0,0,"Binding Done Successfully");
        break;
    }

    logp("DISPATCHER-socketBind",0,0,"Out of the struct finding loop");

    
    if (p == NULL) {//if we got here, it means we didnot get bound
        errorp("DISPATCHER-socketBind", 0, 0, "Unable to bind to port using any IP structure, NO SOCKET IS FORMED");
        if (errno == 98) //already in use
        {   
            logp("DISPATCHER-socketBind",0,0,"Unable to bind because the port is alredy in use");

            logp("DISPATCHER-socketBind",0,0,"Calling clearPort");
            ret = clearPort("4444");
            if (ret == 0){
                logp("DISPATCHER-socketBind",0,0,"Port cleared succesfully, again calling socketBind");
                listener = socketBind(ai);
                logp("DISPATCHER-socketBind",0,0,"socketBind returned Successfully, returning the listener");
                return listener;
            }
            else{
                errorp("DISPATCHER-socketBind",0,0,"Error clearing the port");
                exit(2);
            }
        }
    }

    logp("DISPATCHER-socketBind",0,0,"Socket has been formed in the first attempt, returning listener");
    return listener;
}

int makeSocketForClients(){
    struct addrinfo hints, *ai;
    int rv, listener;

    //get us a socket and bind it
    logp("DISPATCHER-makeSocketForClients",0,0,"Initializing the struct sockaddr");
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    logp("DISPATCHER-makeSocketForClients",0,0,"Calling getaddrinfo");
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        errorp("DISPATCHER-makeSocketForClients",0,0,"Error in getting struct sockaddr from getaddrinfo");
        debugp("DISPATCHER-makeSocketForClients",0,0,gai_strerror(rv));
        exit(1);
    }

    logp("DISPATCHER-makeSocketForClients",0,0,"Calling socketBind");
    listener = socketBind(ai);
    logp("DISPATCHER-makeSocketForClients",0,0,"Successfully made the socket and bounded to the port");
   
    logp("DISPATCHER-makeSocketForClients",0,0,"Freeing the unusable structure memory");
    freeaddrinfo(ai); // all  done with this
    
    //listen
    logp("DISPATCHER-makeSocketForClients",0,0,"Calling listen");
    if (listen(listener, LISTEN_QUEUE_SIZE) == -1) {
        errorp("DISPATCHER-makeSocketForClients",0,0,"Unable to listen");
        debugp("DISPATCHER-makeSocketForClients",1,errno,NULL);
        exit(3);
    }

    //Now lets do the server stuff
    logp("DISPATCHER-makeSocketForClients",0,0,"Socket is listening Successfully, returning listener");
    return listener;
}

void contactPlayer(char* plid, int fd_to_send){
    int socket_fd, recvdBytes;
    printf("in contactPlayer\n");
//    if((fd_to_send = open("vi",O_RDONLY)) < 0)
//        printf("vi open failed");     
    socket_fd = unixClientSocket();
    send_fd(socket_fd, fd_to_send, plid);
    recvdBytes = send(socket_fd, plid, 8, 0);
    if (recvdBytes != 8) {
        errorp("DISPATCHER-contact-Player:", 0, 0, "Unable to send complete plid");
    }
}

int unixClientSocket(){
    logWrite(1,"started"); 

    struct sockaddr_un address;
    int  socket_fd;

    if( (socket_fd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
        printf("socket() failed\n");
        return 1;
    }

    /* start with a clean address structure */
    memset(&address, 0, sizeof(struct sockaddr_in));

    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, sizeof(address.sun_path)-1, "./demo_socket");

    if(connect(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_in)) != 0) {
        printf("connect() failed\n");
        return 1;
    }

    return socket_fd;
}

int send_err(int fd, int errcode, const char *msg)
{
    int     n;

    if ((n = strlen(msg)) > 0)
        if (write(fd, msg, n) != n)    /* send the error message */
            return(-1);

    if (errcode >= 0)
        errcode = -1;   /* must be negative */

    if (send_fd(fd, errcode, NULL) < 0) //NULL for plid
        return(-1);

    return(0);
}

int send_fd(int fd, int fd_to_send, char* plid)
{

    ssize_t temp;
    //struct iovec    iov[2];//second is for sneding plid
    struct iovec    iov[1];//second is for sneding plid
    struct msghdr   msg;
    char            buf[2]; /* send_fd()/recv_fd() 2-byte protocol */

    iov[0].iov_base = buf;
    iov[0].iov_len  = 2;
    // iov[1].iov_base = plid;
    // iov[1].iov_len  = 8;
    msg.msg_iov     = iov;
    // msg.msg_iovlen  = 2;
    msg.msg_iovlen  = 1;
    msg.msg_name    = NULL;
    msg.msg_namelen = 0;
    if (fd_to_send < 0) {
        msg.msg_control    = NULL;
        msg.msg_controllen = 0;
        buf[1] = -fd_to_send;   /* nonzero status means error */
        if (buf[1] == 0)
            buf[1] = 1; /* -256, etc. would screw up protocol */
    } else {
        if (cmptr == NULL && (cmptr = malloc(CONTROLLEN)) == NULL)
            return(-1);
        cmptr->cmsg_level  = SOL_SOCKET;
        cmptr->cmsg_type   = SCM_RIGHTS;
        cmptr->cmsg_len    = CONTROLLEN;
        msg.msg_control    = cmptr;
        msg.msg_controllen = CONTROLLEN;
        printf("msg_controllen(%d)\n",CONTROLLEN );
        *(int *)CMSG_DATA(cmptr) = fd_to_send;     /* the fd to pass */
        buf[1] = 0;          /* z ero status means OK */
    }
    buf[0] = 0;              /* null byte flag to recv_fd() */
    printf("before sendmsg \n");
    temp = sendmsg(fd, &msg, 0);
    if (temp != 2)
    {
        printf("inside sendmsg condition %d\n",temp);
        return(-1);
    }
    printf("after sendmsg %d\n",temp);
    return(0);

}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}





void* SocketHandler(void* lp) {
    logp("DISPATCHER-SocketHandler",0,0,"Stating new thread");
	
    int fd = (long)lp;

    char identity[40];
    sprintf(identity, "DISPATCHER-SocketHandler-fd: %d -", fd);

    logp(identity,0,0,"Successfully gained the identity");

    char choice[1], plid[9];
    int recvdBytes;

    logp(identity,0,0,"receiving the first choice of the client");    
    if ((recvdBytes = recv(fd, choice, sizeof(choice), 0)) == -1) {//whether want to play(login), or as audience(no login)
        fprintf(stderr, "Error receiving data %d\n", errno);
    }
    //TODO - need to check if connection get closed.
    switch(choice[0])
    {
        case 'a':
            int blabla;
            if( (blabla = login(fd, plid)) == 0){
                contactPlayer( plid, fd);
            }else{
                printf("Oh my god\n");
            }
            //printf("login ret %d\n",blabla );
            break;
    }
	return ((void*)0);
}

int login(int fd, char* plid)
{   
    char loginInfo[25], username[9], password[16];
    int loginInfo_len = sizeof(loginInfo);
    int recvdBytes, ret;
    if ((recvdBytes = recv(fd, loginInfo, loginInfo_len, 0)) == -1) {
        fprintf(stderr, "Error receiving data %d\n", errno);
    }

    //printf("Hello Hello rcv(%d)\n%s \n",recvdBytes,loginInfo);
    
    sscanf(loginInfo, "%s%s",username, password); //this is wrong as no \n afterusername

    //printf("------%s\n",password );

    PGconn *conn = NULL;
    
    conn = ConnectDB("postgres","123321","bridge","127.0.0.1","5432");

    if (conn != NULL) {
        ret = login_check(conn, username, password);
        //printf("received return val fro login_check %d\n",ret);
        CloseConn(conn);
        strcpy(plid, username);
        return ret;
    }
}


