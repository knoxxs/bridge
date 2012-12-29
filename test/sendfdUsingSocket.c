#include "accessories.h"


#define CONTROLLEN  CMSG_LEN(sizeof(int))


static struct cmsghdr   *cmptr = NULL;  /* malloc'ed first time */ 
int send_err(int fd, int errcode, const char *msg);
int send_fd(int fd, int fd_to_send);

int main(int argc, char const *argv[])
{   
    logp(1,"started"); 
    int fd_to_send;
    if((fd_to_send = open("vi",O_RDONLY)) < 0)
        printf("vi open failed"); 

    struct sockaddr_un address;
    int  socket_fd, nbytes;
    char buffer[256]; 

    socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if(socket_fd < 0)
    {
        printf("socket() failed\n");
        return 1;
    }

    /* start with a clean address structure */
    memset(&address, 0, sizeof(struct sockaddr_un));

    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, sizeof(address.sun_path)-1, "./demo_socket");

    if(connect(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) != 0)
    {
        printf("connect() failed\n");
        return 1;
    }

    nbytes = snprintf(buffer, 256, "hello from a client");
//    write(socket_fd, buffer, nbytes);

  //  nbytes = read(socket_fd, buffer, 256);
    buffer[nbytes] = 0;

    //printf("MESSAGE FROM SERVER: %s\n", buffer);

    //sending the file descriptor    
    printf("From send_fd %d \n",send_fd(socket_fd,fd_to_send));
    printf("Main end");
    close(socket_fd);
    
    exit(0);

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
