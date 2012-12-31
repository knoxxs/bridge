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
#include "myregex.h"
#include "helper.h"

#define PLAYER_PROCESS_NAME "player"
#define LISTEN_QUEUE_SIZE 10
#define CONTROLLEN  CMSG_LEN(sizeof(int))
#define DATABASE_USER_NAME "postgres"
#define DATABASE_PASSWORD "123321"
#define DATABASE_NAME "bridge"
#define DATABASE_IP "127.0.0.1"
#define DATABASE_PORT "5432"
#define UNIX_SOCKET_FILE_DIS_TO_PLA "./demo_socket"
#define LOG_PATH "./log"
#define IDENTITY_SIZE 40


void *get_in_addr(struct sockaddr*);
int makeSocketForClients();
int socketBind(struct addrinfo *ai);
void* SocketHandler(void* lp);
int login(int fd, char* plid, int len);
void contactPlayer(char*,int,int len);

static struct cmsghdr   *cmptr = NULL;  /* malloc'ed first time */ 
int playerProcId;

int main(int argv, char** argc) {

    int logfile;

    setLogFile(STDOUT_FILENO);
    logp("DISPATCHER-Main", 0,0 ,"Starting");
    if( (logfile = open(LOG_PATH, O_RDWR|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)) < 0 ){
        errorp("DISPATCHER-Main",0,0,"Error Opening logfile");
        debugp("DISPATCHER-Main",1,1,"");
    }
    setLogFile(logfile);

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
    sprintf(buf, "The ID - %d", playerProcId);
    debugp("DISPATCHER-main",0,0,buf);
    
    //connecting to the database
    logp("DISPATCHER-main",0,0,"Connecting to the database");
    if (ConnectDB(DATABASE_USER_NAME, DATABASE_PASSWORD, DATABASE_NAME, DATABASE_IP, DATABASE_PORT, "DISPATCHER-main") == 0){
        logp("DISPATCHER-main",0,0,"Connected to database Successfully");
    }else {
        logp("DISPATCHER-main",0,0,"Unable to connect ot the database");
    }

    logp("DISPATCHER-main",0,0,"Enetering the everlistening while loop");
    while (1) { 
        logp("DISPATCHER-main",0,0,"Waiting for the connection");
	addrlen = sizeof(clientaddr);
        
        logp("DISPATCHER-main",0,0,"Calling accept");
        if((newfd = accept(listener, (struct sockaddr *)&clientaddr, &addrlen)) == -1){
            errorp("DISPATCHER-main",0,0,"unable to accept the connection");
            debugp("DISPATCHER-main",1,errno,"");
		sprintf(buf,"fd %d, size %d err %d",listener,addrlen, errno);

		logp("Dispatcher-main",0,0,buf);
            continue;
        }
        logp("DISPATCHER-main",0,0,"Accepted New Connection");
        
        logp("DISPATCHER-main",0,0,"Calling inet_ntop");
        inet_ntop(clientaddr.ss_family, get_in_addr((struct sockaddr *)&clientaddr), remoteIP, INET6_ADDRSTRLEN);
        sprintf(buf,"Got new connection from IP - %s", remoteIP);
        logp("DISPATCHER-main",0,0,buf);
    
        logp("DISPATCHER-main",0,0,"Making newfd as thread argument compatible");
        thread_arg=(void *)newfd;

        logp("DISPATCHER-main",0,0,"Calling pthread");
        if((err = pthread_create(&thread_id, NULL, SocketHandler,thread_arg ))!=0) {
            errorp("DISPATCHER-main",0,0,"Unable to create the thread");
            debugp("DISPATCHER-main",1,err,"");
        }
        
        logp("DISPATCHER-main",0,0,"Calling pthread_detach");
        if ((err = pthread_detach(thread_id)) != 0){
            errorp("DISPATCHER-main",0,0,"Unable to detach the thread");
            debugp("DISPATCHER-main",1,err,"");
        }
    }

    //TODO graceful exit
    logp("DISPATCHER-main",0,0,"Closing database connection");
    CloseConn("DISPATCHER-main");


    logp("DISPATCHER-main",0,0,"after the infinite while loop - THIS IS IMPOSSIBLE");
    return -1;
}

void *get_in_addr(struct sockaddr *sa) {

    if (sa->sa_family == AF_INET) {
        logp("DISPATCHER-get_in_addr",0,0,"Returning INET address");
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }
    logp("DISPATCHER-get_in_addr",0,0,"Returning INET6 address");
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
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
            debugp("DISPATCHER-socketBind",1,errno,"");
            continue;
        }
        logp("DISPATCHER-socketBind",0,0,"Socket formed succesfully");
        
        logp("DISPATCHER-socketBind",0,0,"Calling setsockopt");
        if ((setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int)))) {
            errorp("DISPATCHER-socketBind",0,0,"Unable to manipulate the socket");
            debugp("DISPATCHER-socketBind",1,errno, "");
            exit(1);
        }
        logp("DISPATCHER-socketBind",0,0,"setsockopt done succesfully");
        
        logp("DISPATCHER-socketBind",0,0,"Calling bind");
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            errorp("DISPATCHER-socketBind",0,0,"Unable to bind the socket");
            debugp("DISPATCHER-socketBind",1,errno,"");
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
        debugp("DISPATCHER-makeSocketForClients",1,errno,"");
        exit(3);
    }

    //Now lets do the server stuff
    logp("DISPATCHER-makeSocketForClients",0,0,"Socket is listening Successfully, returning listener");
    return listener;
}

void* SocketHandler(void* lp) {
    logp("DISPATCHER-SocketHandler",0,0,"Stating new thread");
    
    int fd = (long)lp;

    char identity[IDENTITY_SIZE], buf[100];
    sprintf(identity, "DISPATCHER-SocketHandler-fd: %d -", fd);

    logp(identity,0,0,"Successfully gained the identity");

    char choice[1], plid[9]; 
    int choice_len = sizeof(choice);//this is importnat becz recvall takes ppointer to int
    int ret;

    logp(identity,0,0,"receiving the first choice of the client");
    if(recvall(fd, choice, &choice_len, 0) != 0){ //whether want to play(login), or as audience(no login)
        logp(identity,0,0,"Error receiving the first choice of the client");
    }

    logp(identity,0,0,"Entering the choice Select switch");
    switch(choice[0])
    {
        case 'a':
            sprintf(buf, "plid_len(%d), username_len(), plid - %d",sizeof(plid), plid);
            logp(identity,0,0,buf);

            logp(identity,0,0,"User entered the choice 'a' and calling login");
	   		sprintf(buf, "plid_len(%d), username_len(), plid - %d",sizeof(plid), plid);
   	 		debugp(identity,0,0,buf);
            if( (ret = login(fd, plid, sizeof(plid))) == 0){
                sprintf(buf,"Player id is %s and Contacting player",plid);
                logp(identity,0,0,buf);
                contactPlayer( plid, fd, sizeof(plid));
                logp(identity,0,0,"Contacted To player succesfully");
            }else{
                logp(identity,0,0,"Incorrect login Credentials");
            }
            break;
        default:
            errorp(identity,0,0,"User entered the wrong choice");
    }

    logp(identity,0,0,"Returning From the thread");
    return ((void*)0);
}

int login(int fd, char* plid, int len){
    char loginInfo[25], username[9], password[16];
    int loginInfo_len = sizeof(loginInfo);
    int ret;

    char identity[IDENTITY_SIZE], buf[100];
    sprintf(identity, "DISPATCHER-login-fd: %d -", fd);

    sprintf(buf, "plid_len(%d), username_len(%d), plid - %d",len, sizeof(username), plid);
    logp(identity,0,0,buf);
	
	sprintf(buf, "plid_len(%d), username_len(%d), plid - %d",len, sizeof(username), plid);
   	debugp(identity,0,0,buf);
	
    logp(identity,0,0,"Calling recvall to recv login credentials");
    if ((ret = recvall(fd, loginInfo, &loginInfo_len, 0)) != 0) {
        errorp(identity,0,0,"Unable to recv login credentials");
        debugp(identity,1,errno,"");
    }

    sscanf(loginInfo, "%s%s",username, password); //this is wrong as no \n afterusername
    sprintf(buf,"recvd login credential with username - %s", username);
    logp(identity,0,0,buf);

    logp(identity,0,0,"Calling login_check");
    ret = login_check(username, password, identity);
    sprintf(buf,"Recvd (%d) from login_check", ret);
    logp(identity,0,0,buf);
    
    strcpy(plid, username);
    sprintf(buf, "plid_len(%d), username_len(%d)",sizeof(plid), sizeof(username));
    logp(identity,0,0,buf);

    logp(identity,0,0,"Returning from login");
    return ret;
}

void contactPlayer(char* plid, int fd_to_send, int len){
    int socket_fd;

    char identity[IDENTITY_SIZE], buf[100];
    sprintf(identity, "DISPATCHER-contact-Player-fd: %d -", fd_to_send);    

    logp(identity,0,0,"Inside contactPlayer and calling unixClientSocket");
    sprintf(buf,"DISPATCHER-unixClientSocket-fd: %d -",fd_to_send);
    socket_fd = unixClientSocket(UNIX_SOCKET_FILE_DIS_TO_PLA,buf,sizeof(buf));
    sprintf(buf,"Recved socket-fd: %d -",socket_fd);
    logp(identity,0,0,buf);

    logp(identity,0,0,"Calling send_fd");
    send_fd(socket_fd, fd_to_send, plid, len, identity);
    logp(identity,0,0,"fd sent Successfully");

    sprintf(buf, "Sending plid to player process ,plid_length(%d)",len);
    logp(identity,0,0,buf);
    if( sendall(socket_fd, plid, &len, 0) != 0){
        errorp(identity, 0, 0, "Unable to send complete plid");
        debugp(identity,1,errno,"");
    }
}
