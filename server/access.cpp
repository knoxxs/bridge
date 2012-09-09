#include "access.h"

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

void errorp(char *where, int boolean, int errn,char *what)
{
	char errmsg[21+strlen(where)];
	strcpy(errmsg,"Where - ");
	strcat(errmsg,where);
	strcat(errmsg,"  |  Error - ");
	
	if(boolean == 1)//we got error number
	{
		strcat(errmsg,strerror(errn));
		//TO-DO: need to add actual number to the message
		//fprintf(stderr,"ERROR - In %s and error is %s\n",where ,strerror(errn));
		logp(3,errmsg);
	}
	else if(boolean == 0)//we got a message
	{
		strcat(errmsg,what);
		//fprintf(stderr,"ERROR - In %s and error is %s\n",where ,what);
		logp(3,errmsg);
	}
	else//we got nothing
	{
		strcat(errmsg,"No Message");
		//fprintf(stderr,"ERROR - In %s\n",where);
		logp(3,errmsg);	
	}
}

int strintToInt(char *str, int len)
{
	int iter, pow_iter, num=0;
	for(iter = 0, pow_iter = len-1; iter < len ; iter++, pow_iter--)
	{
		num += (str[iter]-48)* ((int) pow(10,pow_iter));
	}
	return num;

}