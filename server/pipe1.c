#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#define MSGSIZ 63
char *fifo = "fifo";
main(int argc, char **argv)
{
	int fd, j, nwrite;
	char msgbuf[MSGSIZ+1];
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
		if((nwrite = write(fd, msgbuf, MSGSIZ+1)) == -1)
			printf("message write failed");
	}
	exit(0);

}
