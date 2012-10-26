#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#define MSGSIZ 63

char *fifo = "fifo";

main(int argc, char **argv)
{
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
	for (; ; )
	{
		if(read(fd, msgbuf, MSGSIZ+1) < 0)
			printf("message read failed");
		/*
		* print out message; in real life
		* something more interesting would
		* be done
		*/
		printf("message received:%s\n",msgbuf);
	}
}
