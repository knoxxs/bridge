#include "access.h"
#include "psql.h"
#include "myregex.h"

#define CONTROLLEN  CMSG_LEN(sizeof(int))
static struct cmsghdr   *cmptr = NULL;  /* malloc'ed first time */ 

void* SocketHandler(void* lp);
int listenBind(struct addrinfo *ai);
int login(int fd, char* plid);
//get sockaddr , IPv4 or IPv6:
void contactPlayer(char*,int);
int send_fd(int, int);
int unixClientSocket();
int send_err(int, int, const char *);

void contactPlayer(char* plid, int fd_to_send){
    int socket_fd;
    printf("in contactPlayer\n");
    if((fd_to_send = open("vi",O_RDONLY)) < 0)
        printf("vi open failed");     
    socket_fd = unixClientSocket();
    send_fd(socket_fd, fd_to_send);
}

int unixClientSocket(){
    logp(1,"started"); 

    struct sockaddr_un address;
    int  socket_fd;

    if( (socket_fd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
        printf("socket() failed\n");
        return 1;
    }

    /* start with a clean address structure */
    memset(&address, 0, sizeof(struct sockaddr_un));

    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, sizeof(address.sun_path)-1, "./demo_socket");

    if(connect(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) != 0) {
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

    if (send_fd(fd, errcode) < 0)
        return(-1);

    return(0);
}

int send_fd(int fd, int fd_to_send)
{

    ssize_t temp;
    struct iovec    iov[1];
    struct msghdr   msg;
    char            buf[2]; /* send_fd()/recv_fd() 2-byte protocol */

    iov[0].iov_base = buf;
    iov[0].iov_len  = 2;
    msg.msg_iov     = iov;
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

int playerProcId;

int main(int argv, char** argc) {
    //printf(PORT);
    struct sockaddr_in my_addr;

    int listener; // listening the socket descriptor
    int newfd; //newly accept()ed file descriptor
    struct sockaddr_storage clientaddr; // client address
    socklen_t addrlen;
    void **thread_ret;

    char buf[256]; //buffer for client
    char remoteIP[INET6_ADDRSTRLEN];

    int i, j, rv;
    pthread_t thread_id = 0;
	void *thread_arg;
    struct addrinfo hints, *ai;
    int err;


    //get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        //fprintf(stderr, "selectserver : %s\n", gai_strerror(rv));
        errorp("DISPATCHER-getaddrinfo:", 1, rv, "a");
        exit(1);
    }
	logp(4,"struct addrinfo has been formed succesfully");
    
    listener = listenBind(ai);

   
    freeaddrinfo(ai); // all  done with this
	logp(4,"memory freed successfully");
	
    //listen
    if (listen(listener, 10) == -1) {
        errorp("DISPATCHER-listener:", 0, 0, "Unable to connect to reader");
        exit(3);
    }
    //Now lets do the server stuff
	logp(4,"socket is listening");
	logp(1,"socket is listening");

    //Finding pid of player process
    

    playerProcId = processIdFinder("player");
    printf("Player proc ID is %d\n", playerProcId);

    while (1) {
    	
    	logp(1,"Waiting for connections");
    	
        if((newfd = accept(listener, (struct sockaddr *)&clientaddr, &addrlen)) == -1){
        	errorp("DISPATCHER-accept:", 0, 0, "Unable to accept one connection to reader");
			//continue;
		}

		logp(4,"accepted new connection");
		
		inet_ntop(clientaddr.ss_family, get_in_addr((struct sockaddr *)&clientaddr), remoteIP, INET6_ADDRSTRLEN);
		printf("server : got connection from %s\n", remoteIP);
		
		strcat(remoteIP,": Got new connection");
		logp(1,remoteIP);
		
		thread_arg=(void *)newfd;
		if((err = pthread_create(&thread_id, NULL, SocketHandler,thread_arg ))!=0)
		{
			errorp("DISPATCHER-pthrad_create:", 1, err, NULL);
		}
            
            if ((err = pthread_detach(thread_id)) != 0)
            {
            	errorp("DISPATCHER-pthread_detach:", 1, err, NULL);
            }
    }

    return 0;
}

void* SocketHandler(void* lp) {
	
    int fd = (long)lp;
	
	logp(1,"New thread created succesfully");
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
    // char buffer[6];
    // int buffer_len = 6;
    // int bytecount;
    // memset(buffer, 0, buffer_len);
    // if ((bytecount = recv(fd, buffer, buffer_len, 0)) == -1) {
    //     fprintf(stderr, "Error receiving data %d\n", errno);
    //     //goto FINISH;
    // }
    // printf("Received bytes %d\nReceived string \"%s\"\n", bytecount, buffer);
    // //strcat(buffer, "SERVER");
	
    // if ((bytecount = send(fd, buffer, strlen(buffer), 0)) == -1) {
    //     fprintf(stderr, "Error sending data %d\n", errno);
    //     //goto FINISH;
    // }
    // printf("Sent bytes %d\n", bytecount);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    char choice[1], plid[9];
    int choice_len = sizeof(choice);
    int recvdBytes;
    if ((recvdBytes = recv(fd, choice, choice_len, 0)) == -1) {
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

int listenBind(struct addrinfo *ai)
{
    struct addrinfo  *p;
    int yes = 1, ret;
    int listener;

    for (p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) {
            continue;
        }
        logp(4,"socket formed succesfully");
        //lose the pesky "address already in use " error message
        //setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if ((setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int)))) {
            errorp("DISPATCHER-setsockopt:", 1, errno, NULL);
            //printf("Error setting options %d\n", errno);
            exit(1);
        }
        logp(4,"setsockopt formed succesfully");
        
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            errorp("DISPATCHER-bind:", 1, errno, "");
            continue;
        }
        logp(4,"binding has been done");
        break;
    }

    printf("In listen bind\n");

    //if we got here, it means we didnot get bound
    if (p == NULL) {
        //fprintf(stderr, "selectserver: failed to bind\n");
        errorp("DISPATCHER-bindcheck:", 0, 0, "Unable to bind to port using any IP");
        if (errno == 98) //already in use
        {   
            ret = clearPort("4444");
            if (ret == 0){
                //printf("a\n");
                listener = listenBind(ai);
                return listener;
            }
            else{
                printf("error clearPort c\n");
                exit(2);
            }
        }
    }

    printf("In listen bind ___ out\n");

    return listener;
}
