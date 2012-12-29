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
#include "helper.h"

static struct cmsghdr   *cmptr = NULL;      /* malloc'ed first time */

int unixSocket(char* sock, char * identity, int listenQSize){
    struct sockaddr_un address;
    int socket_fd;
    socklen_t address_length;
    char buf[100];

    char cmpltIdentity[CMPLT_IDENTITY_SIZE];
    strcpy(cmpltIdentity, identity);
    strcat(cmpltIdentity,"-unixSocket-");


    logp(cmpltIdentity,0,0,"Inside unixSocket and calling socket");
    if( (socket_fd = socket(PF_UNIX, SOCK_STREAM, 0) ) < 0 ) {
        errorp(cmpltIdentity,0,0,"Unable to create the socket");
        debugp(cmpltIdentity,1,errno,"");
        return -1;
    } 


    unlink(sock);

    /* start with a clean address structure */
    memset(&address, 0, sizeof(struct sockaddr_un));

    logp(cmpltIdentity,0,0,"Making struct");
    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, sizeof(address.sun_path)-1, sock);

    logp(cmpltIdentity,0,0,"Calling bind");
    if(::bind(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) != 0){
        errorp(cmpltIdentity,0,0,"Unable to bind to the socket");
        debugp(cmpltIdentity,1,errno,"");
        return -1;
    }

    logp(cmpltIdentity,0,0,"calling listen");
    if(listen(socket_fd,listenQSize ) != 0) {
        errorp(cmpltIdentity,0,0,"Unable to create the socket");
        debugp(cmpltIdentity,1,errno,"");
        return -1;
    }

    sprintf(buf,"returning socket_fd %d",socket_fd);
    logp(cmpltIdentity,0,0,buf);
    return socket_fd;
}

int getPlayerInfo(char *plid, char *name, char *team, int identity_fd ,int p_len, int n_len, int t_len, char *identity){
    int ret;
    char buf[100];

    sprintf(buf, "-getPlayerInfo-fd: %d -", identity_fd);

    char cmpltIdentity[CMPLT_IDENTITY_SIZE];
    strcpy(cmpltIdentity, identity);
    strcat(cmpltIdentity,buf);
    
    sprintf(buf, "Connected to data Succesfully and calling getPlayerInfoFromDb with plid %s",plid);
    logp(cmpltIdentity,0,0,buf);
    ret = getPlayerInfoFromDb(plid, name, team, identity);
    logp(cmpltIdentity,0,0,"Returned from getPlayerInfoFromDb");

    sprintf(buf,"Returning from getPlayerInfo with ret value (%d)",ret);
    logp(cmpltIdentity,0,0,buf);
    return ret;// 0 - done, -1 - error in getPlayerInfoFromDb
}

int recv_fd(int fd, ssize_t (*userfunc)(int, const void *, size_t), char *plid, int len, char *identity) {
    int             newfd, nr, status;
    char            *ptr;
    char            buf[MAXLINE];
    //struct iovec    iov[2];
    struct iovec    iov[1];
    struct msghdr   msg;

    char tempbuf[100];
    sprintf(tempbuf, "-recv_fd-fd: %d -", fd);

    char cmpltIdentity[CMPLT_IDENTITY_SIZE];
    strcpy(cmpltIdentity, identity);
    strcat(cmpltIdentity,tempbuf);

    logp(cmpltIdentity,0,0,"Entering the infite for loop");
    status = -1;
    for ( ; ; ) 
    {
        logp(cmpltIdentity,0,0,"Stuffing iovec");
        iov[0].iov_base = buf;
        iov[0].iov_len  = sizeof(buf);
        //iov[1].iov_base = plid;
        //iov[1].iov_len  = 8;

        logp(cmpltIdentity,0,0,"Filling msghdr");
        msg.msg_iov     = iov;
        //msg.msg_iovlen  = 2;
        msg.msg_iovlen  = 1;
        msg.msg_name    = NULL;
        msg.msg_namelen = 0;

        logp(cmpltIdentity,0,0,"Allocating memory");
        if (cmptr == NULL && (cmptr = malloc(CONTROLLEN)) == NULL)
            return(-1);

        logp(cmpltIdentity,0,0,"Assinging allocated memory to msghdr");
        msg.msg_control    = cmptr;

        sprintf(tempbuf,"Adding lenght of allocated memory, lenght - controllen(%d)",CONTROLLEN);
        logp(cmpltIdentity,0,0,tempbuf);
        msg.msg_controllen = CONTROLLEN;
        
        logp(cmpltIdentity,0,0,"Calling recvmsg");
        nr = recvmsg(fd, &msg, 0);
        if (nr < 0) {
            errorp(cmpltIdentity,0,0,"Unable to recv the msg");
            debugp(cmpltIdentity,1,errno,"");
            return -1;
        } else if (nr == 0) {
            logp(cmpltIdentity,0,0,"Unable to recv the msg becz connection is closed by client");
            return -1;
        }
        /*
        * See if this is the final data with null & status.  Null
        * is next to last byte of buffer; status byte is last byte.
        * Zero status means there is a file descriptor to receive.
        */
        logp(cmpltIdentity,0,0,"Entering the data traversing for loop");
        for (ptr = buf; ptr < &buf[nr]; ) 
        {
            logp(cmpltIdentity,0,0,"Inside traversing for loop");
            if (*ptr++ == 0) 
            {
                logp(cmpltIdentity,0,0,"Recvd First zero");
                if (ptr != &buf[nr-1])
                    errorp(cmpltIdentity,0,0,"Message Format error");
                status = *ptr & 0xFF;  /* prevent sign extension */
                if (status == 0) {
                    sprintf(tempbuf,"msg_controllen recvd is(%d) and must be is(%d)\n",msg.msg_controllen, CONTROLLEN );
                    logp(cmpltIdentity,0,0,tempbuf);
                    if (msg.msg_controllen != CONTROLLEN)
                        logp(cmpltIdentity,0,0,"Status =0 but no fd");
                    newfd = *(int *)CMSG_DATA(cmptr);
                    sprintf(tempbuf, "New fd extracted is %d",newfd);
                    logp(cmpltIdentity,0,0,tempbuf);
                } else {
                    logp(cmpltIdentity,0,0,"assigning newfd = error status");
                    newfd = -status;
                }
                nr -= 2;
            }
        }

        logp(cmpltIdentity,0,0,"Checking whether fd recvd succesfully or not");
        if (nr > 0 && (*userfunc)(STDERR_FILENO, buf, nr) != nr)
            logp(cmpltIdentity,0,0,"Error extracting error from the msg");
            //return(-1);
        if (status >= 0){    /* final data has arrived */
            logp(cmpltIdentity,0,0,"Returning new fd");
            return(newfd);  /* descriptor, or -status */
        }
    }
}

ssize_t errcheckfunc(int a,const void *b, size_t c, char* identity){
    char cmpltIdentity[CMPLT_IDENTITY_SIZE];
    strcpy(cmpltIdentity, identity);
    strcat(cmpltIdentity,"-errcheckfunc-");

    logp(cmpltIdentity ,0,0,"Inside error check function");
    return 0;
}

int unixClientSocket(char* sock, char* identity, int len){
    struct sockaddr_un address;
    int  socket_fd;

    char cmpltIdentity[CMPLT_IDENTITY_SIZE];
    strcpy(cmpltIdentity, identity);
    strcat(cmpltIdentity,"-unixClientSocket-");

    logp(cmpltIdentity,0,0,"Calling socket");
    if( (socket_fd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
        errorp(cmpltIdentity,0,0,"Unable to make the socket");
        debugp(cmpltIdentity,1,errno,"");
        return -1;
    }

    /* start with a clean address structure */
    logp(cmpltIdentity,0,0,"Cleaning the struct");
    memset(&address, 0, sizeof(struct sockaddr_un));

    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, sizeof(address.sun_path)-1, sock);

    logp(cmpltIdentity,0,0,"Calling Connect");
    if(connect(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) != 0) {
        errorp(cmpltIdentity,0,0,"Unable to connect the socket");
        debugp(cmpltIdentity,1,errno,"");
        return -1;
    }

    logp(cmpltIdentity,0,0,"Returning socket_fd");
    return socket_fd;
}

int send_err(int fd, int errcode, const char *msg, int len){
    int n;

    if ((n = strlen(msg)) > 0)
        if (write(fd, msg, n) != n)    /* send the error message */
            return(-1);

    if (errcode >= 0)
        errcode = -1;   /* must be negative */

    if (send_fd(fd, errcode, NULL,0, NULL) < 0) //NULL for plid
        return(-1);

    return(0);
}

int send_fd(int fd, int fd_to_send, char* plid, int len, char* identity){
    ssize_t temp;
    //struct iovec    iov[2];//second is for sending plid
    struct iovec    iov[1];//second is for sneding plid
    struct msghdr   msg;
    char            buf[2]; /* send_fd()/recv_fd() 2-byte protocol */

    char tempbuf[100];

    sprintf(tempbuf, "send_fd-fd: %d -", fd_to_send);    
    char cmpltIdentity[CMPLT_IDENTITY_SIZE];
    strcpy(cmpltIdentity, identity);
    strcat(cmpltIdentity,tempbuf);



    logp(cmpltIdentity,0,0,"Adding bufs to iovec");
    iov[0].iov_base = buf;
    iov[0].iov_len  = 2;
    // iov[1].iov_base = plid;
    // iov[1].iov_len  = 8;

    logp(cmpltIdentity,0,0,"Adding iovec to msghdr");
    msg.msg_iov     = iov;
    // msg.msg_iovlen  = 2;
    msg.msg_iovlen  = 1;
    msg.msg_name    = NULL;
    msg.msg_namelen = 0;


    if (fd_to_send < 0) {
        logp(cmpltIdentity,0,0,"When fd_to_send is < 0, defining the structure of buf according to the protocol");
        msg.msg_control    = NULL;
        msg.msg_controllen = 0;
        buf[1] = -fd_to_send;   /* nonzero status means error */
        if (buf[1] == 0)
            buf[1] = 1; /* -256, etc. would screw up protocol */
    } else {
        logp(cmpltIdentity,0,0,"When fd_to_send > 0, assigning memory");
        if (cmptr == NULL && (cmptr = malloc(CONTROLLEN)) == NULL)
            return(-1);
        logp(cmpltIdentity,0,0,"Giving rights");
        cmptr->cmsg_level  = SOL_SOCKET;
        cmptr->cmsg_type   = SCM_RIGHTS;
        cmptr->cmsg_len    = CONTROLLEN;
        msg.msg_control    = cmptr;
        msg.msg_controllen = CONTROLLEN;

        sprintf(tempbuf,"Adding data with controllen(%d)",CONTROLLEN);
        logp(cmpltIdentity,0,0,tempbuf);
        *(int *)CMSG_DATA(cmptr) = fd_to_send;     /* the fd to pass */
        buf[1] = 0;          /* zero status means OK */
    }


    buf[0] = 0;              /* null byte flag to recv_fd() */
    logp(cmpltIdentity,0,0,"Sending the fd and the msg");
    temp = sendmsg(fd, &msg, 0);
    if (temp != 2){
        if(temp == -1){
            errorp(cmpltIdentity,0,0,"Unable to send the data");
            debugp(cmpltIdentity,1,errno,NULL);
        }else {
            errorp(cmpltIdentity,0,0,"Unable to send complete data");
        }

        logp(cmpltIdentity,0,0,"Returning Unsuccessful");
        return(-1);
    }

    logp(cmpltIdentity,0,0,"Returning Successfully");
    return(0);
}
