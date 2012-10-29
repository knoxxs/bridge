#include <stropts.h>
#include "accesories.c"
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

#define MSGSIZ 63
char *fifo = "fifo";

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
    char    buf[2];     /* send_fd()/recv_fd() 2-byte protocol */
    
    buf[0] = 0;         /* null byte flag to recv_fd() */
    if (fd_to_send < 0) {
        buf[1] = -fd_to_send;   /* nonzero status means error */
        if (buf[1] == 0)
            buf[1] = 1; /* -256, etc. would screw up protocol */
    } else {
        buf[1] = 0;     /* zero status means OK */
    }
    //printf("From the write %d\n",buf[0]);
    if (write(fd, buf, 2) != 2)
        return(-1);

    if (fd_to_send >= 0)
        if (ioctl(fd, I_SENDFD, fd_to_send) < 0)
        {
            printf("Eroor ::: %s\n",strerror(errno));
            return(-1);
        }
    return(0);
}




int main(int argc, char const *argv[])
{
    int fd, j, nwrite;
    char msgbuf[MSGSIZ+1];
    int fd_to_send;


    if((fd_to_send = open("vi",O_RDONLY)) < 0)
        printf("vi open failed");

    if(argc < 2)
    {
        fprintf(stderr, "Usage: sendmessage msg ... \n");
        exit(1);
    }
    /* open fifo with O_NONBLOCK set */
    if((fd = open(fifo, O_WRONLY | O_NONBLOCK)) < 0)
        printf("fifo open failed");
    
    /* send messages */
    for (j = 1; j < argc; j++)
    {
        if(strlen(argv[j]) > MSGSIZ)
        {
            fprintf(stderr, "message too long %s\n", argv[j]);
            continue;
        }
        strcpy(msgbuf, argv[j]);
        if((nwrite = write(fd, msgbuf, 6)) == -1)
            printf("message write failed");
    }

    printf("From send_fd %d \n",send_fd(fd,fd_to_send));
    
    exit(0);

}