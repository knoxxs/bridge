#include "player_helper.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>
#include <errno.h>
#include <pthread.h>

#define MAXLINE 2

/* size of control buffer to send/recv one file descriptor */
#define CONTROLLEN  CMSG_LEN(sizeof(int))
 
static struct cmsghdr   *cmptr = NULL;      /* malloc'ed first time */

int unixSocket(){
	struct sockaddr_un address;
	int socket_fd;
	socklen_t address_length;

	
	if( (socket_fd = socket(PF_UNIX, SOCK_STREAM, 0) ) < 0 ) {
		printf("socket() failed\n");
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
