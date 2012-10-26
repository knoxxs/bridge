#include <malloc.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <error.h>
#include <signal.h>

#define PORT "1618" //port we are listening on


int sendall(int fd, char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = send(fd, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
}

int recvall(int fd, char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = recv(fd, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
} 

void logp(int typ, char* msg) // typ --> type(category) of message [1-Normal Log, 2-Warning(any unexpected thing happened), 3-Error, 4-Debugging Log ]
{
	int fd;
	time_t now;
	ssize_t wlength=0;
	char * dat;
	char * str;
	int size = 45+strlen(msg);//14+24+5+sizeof msg+1
	
	str= (char *) malloc(size);
	
	time(&now);//system time in seconds
	dat = ctime(&now); // converting seconds to date-time format
	dat = strtok(dat,"\n");
	
	//Appending type of log
	switch(typ)
	{
	case 1:
		strcpy(str,"__LOG__    |  ");
		strcat(str,dat);
		break;
	case 2:
		strcpy(str,"__WARN__   |  ");
		strcat(str,dat);
		break;
	case 3:
		strcpy(str,"__ERR__    |  ");
		strcat(str,dat);
		break;
	case 4:
		strcpy(str,"__DEBUG__  |  ");
		strcat(str,dat);
		break;
	default:
		strcpy(str,"__UNDEF__  |  ");
		strcat(str,dat);
		break;
	}
	
	
	strcat(str,"  |  ");
	strcat(str,msg);//appending message
	strcat(str,"\n");
	
	fd = open("log", O_WRONLY | O_CREAT | O_APPEND, 0644); // should be opened somewhere else
	if (fd == -1)
		printf("Could not open log - %s\n",strerror(errno));
	else
	{//need to add lock to the file and printing error message
		while ( wlength < strlen(str) )
		{
			wlength = write(fd, str,strlen(str));
			if (wlength == -1)
			{
				printf("Error : writing log\n");
				break;
			}
		}
		
		
	}
}
void errorp(char *where, int boolean, int errn,char *what)
{
	char errmsg[150];
	strcpy(errmsg,"Where - ");
	strcat(errmsg,where);
	strcat(errmsg,"  |  Error - ");

	if(boolean == 1)//we got error number
	{
		strcat(errmsg,strerror(errn));
		//fprintf(stderr,"ERROR - In %s and error is %s\n",where ,strerror(errn));
		logp(3,errmsg);
	}
	else if(boolean == 0)
	{
		strcat(errmsg,what);
		//fprintf(stderr,"ERROR - In %s and error is %s\n",where ,what);
		logp(3,errmsg);
	}
	else
	{
		strcat(errmsg,"No Message");
		//fprintf(stderr,"ERROR - In %s\n",where);
		logp(3,errmsg);	
	}
}


