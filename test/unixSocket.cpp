#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_PATH "./demo_socket"
#define MAXLINE 2
/* size of control buffer to send/recv one file descriptor */
#define CONTROLLEN  CMSG_LEN(sizeof(int))
 
static struct cmsghdr   *cmptr = NULL;      /* malloc'ed first time */

int socketBindListen_Unix() {

	int s;
	struct sockaddr_un local;

	if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	/* start with a clean address structure */
	memset(&local, 0, sizeof(struct sockaddr_un));


	local.sun_family = AF_UNIX;
	strcpy(local.sun_path, SOCK_PATH);
	unlink(local.sun_path);
	len = strlen(local.sun_path) + sizeof(local.sun_family);

	if (bind(s, (struct sockaddr *)&local, len) == -1) {
		perror("bind");
		exit(1);
	}

	if (listen(s, 5) == -1) {
		perror("listen");
		exit(1);
	}

	return s;
}

void logAcceptedConnection(struct* sockaddr_un remote){


}


int acceptConnection_Unix(int s) {
	int s2, t, len;
	struct sockaddr_un remote;

	printf("Waiting for a connection...\n");
	t = sizeof(remote);
	if ((s2 = accept(s, (struct sockaddr *)&remote, &t)) == -1) {
		perror("accept");
		exit(1);
	}
	printf("Connected.\n");

	logAcceptedConnection(&remote);

	return s2;
}

int socketConnect(){

	int s, t, len;
	struct sockaddr_un remote;
	char str[100];

	if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	
	printf("Trying to connect...\n");

	memset(&remote, 0, sizeof(struct sockaddr_un));

	remote.sun_family = AF_UNIX;
	strcpy(remote.sun_path, SOCK_PATH);
	len = strlen(remote.sun_path) + sizeof(remote.sun_family);

	if (connect(s, (struct sockaddr *)&remote, len) == -1) {
		perror("connect");
		exit(1);
	}

	printf("Connected.\n");

	return s;
}


// int fd_to_recv;
// fd_to_recv = recv_fd(connection_fd ,&errcheckfunc);
int recv_fd(int fd, ssize_t (*userfunc)(int, const void *, size_t)) {
    int             newfd, nr, status;
    char            *ptr;
    char            buf[MAXLINE];
    struct iovec    iov[1];
    struct msghdr   msg;

    status = -1;
    for ( ; ; ) 
    {
        iov[0].iov_base = buf;
        iov[0].iov_len  = sizeof(buf);
        msg.msg_iov     = iov;
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

// printf("From send_fd %d \n",send_fd(socket_fd,fd_to_send));
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

void closeServer(int fd){
	close(fd);
	unlink(SOCK_PATH);
}

void closeClient(int fd){
	close(fd);
	unlink(SOCK_PATH);
}