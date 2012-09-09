#include <stropts.h>
#include "accesories.c"
#include <sys/ioctl.h>
#define MAXLINE 10
#define MSGSIZ 63

char *fifo = "fifo";

int recv_fd(int fd, ssize_t (*userfunc)(int, const void *, size_t))
{
    int                 newfd, nread, flag, status;
    char                *ptr;
    char                buf[MAXLINE];
    struct strbuf       dat;
    struct strrecvfd    recvfd;

    status = -1;
    for ( ; ; ) {
        dat.buf = buf;
        dat.maxlen = MAXLINE;
        flag = 0;
        if (getmsg(fd, NULL, &dat, &flag) < 0)
            //err_sys("getmsg error");
            perror("getmsg error");
        nread = dat.len;
        if (nread == 0) {
            //err_ret("connection closed by server");
            perror("connection closed by server");
            return(-1);
        }
        /*
         * See if this is the final data with null & status.
         * Null must be next to last byte of buffer, status
         * byte is last byte. Zero status means there must
         * be a file descriptor to receive.
         */
        for (ptr = buf; ptr < &buf[nread]; ) {
            if (*ptr++ == 0) {
                if (ptr != &buf[nread-1])
                    //err_dump("message format error");
                    perror("message format error");
                 status = *ptr & 0xFF;   /* prevent sign extension */
                 if (status == 0) {
                     if (ioctl(fd, I_RECVFD, &recvfd) < 0)
                         return(-1);
                     newfd = recvfd.fd;  /* new descriptor */
                 } else {
                     newfd = -status;
                 }
                 nread -= 2;
            }
        }
        if (nread > 0)
            if ((*userfunc)(STDERR_FILENO, buf, nread) != nread)
                 return(-1);

        if (status >= 0)    /* final data has arrived */
            return(newfd);  /* descriptor, or -status */
    }
}

ssize_t errcheckfunc(int a,const void *b, size_t c)
{
	return 0;
}


int main(int argc, char const *argv[])
{
	/* code */
	int fd;
	char msgbuf[MSGSIZ+1];
	/* create fifo, if it doesn't already exist */
	if(mkfifo(fifo, 0666) == -1 )
	{
		if(errno != EEXIST)
			printf("receiver: mkfifo");
	}

	/* open fifo for reading and writing */
	if((fd = open(fifo, O_RDWR)) < 0)
		printf ("fifo open failed");
	
	/* receive messages */
//	for (; ; )
	{
		if(read(fd, msgbuf, 6) < 0)
			printf("message read failed");
		/*
		* print out message; in real life
		* something more interesting would
		* be done
		*/
		printf("message received:%s\n",msgbuf);
	}
	//printf("From second read %d\n",read(fd,msgbuf,2));
	//printf("message received:%d\n",msgbuf[0]);
int fd_to_recv;
	fd_to_recv = recv_fd(fd,&errcheckfunc);


		if(read(fd_to_recv,msgbuf,5) < 0)
			printf("message read failed");
		printf("message received:%s\n",msgbuf);

	return 0;
}