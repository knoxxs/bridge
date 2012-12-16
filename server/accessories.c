#include "accessories.h"
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

 
void setLogFile(int log){
	logfile = log;
}

void logWrite(int typ, char* msg) // typ --> type(category) of message [1-Normal Log, 2-Warning(any unexpected thing happened), 3-Error, 4-Debugging Log ]
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
	case LOG_TYPE:
		strcpy(str,"__LOG__    |  ");
		strcat(str,dat);
		break;
	case WARN_TYPE:
		strcpy(str,"__WARN__   |  ");
		strcat(str,dat);
		break;
	case ERROR_TYPE:
		strcpy(str,"__ERR__    |  ");
		strcat(str,dat);
		break;
	case DEBUG_TYPE:
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
	
	//fd = open("log", O_WRONLY | O_CREAT | O_APPEND, 0644); // should be opened somewhere else
	fd = logfile;
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


void makeMessage(int type, int boolean, char* errmsg, int errn, char* what){
	if(boolean == 1) { //we got error number
		strcat(errmsg,strerror(errn));
		//fprintf(stderr,"ERROR - In %s and error is %s\n",where ,strerror(errn));
		logWrite(type,errmsg);
	}
	else if(boolean == 0){ //we got a message
		strcat(errmsg,what);
		//fprintf(stderr,"ERROR - In %s and error is %s\n",where ,what);
		logWrite(type,errmsg);
	}
	else{ //we got nothing
		strcat(errmsg,"No Message");
		//fprintf(stderr,"ERROR - In %s\n",where);
		logWrite(type,errmsg);	
	}
} 

void errorp(char *where, int boolean, int errn,char *what)
{
	char errmsg[21+strlen(where)];
	strcpy(errmsg,"Where - ");
	strcat(errmsg,where);
	strcat(errmsg,"  |  Error - ");

	makeMessage(ERROR_TYPE, boolean, errmsg, errn, what);
}

void logp(char *where, int boolean, int errn,char *what)
{
	char errmsg[21+strlen(where)];
	strcpy(errmsg,"Where - ");
	strcat(errmsg,where);
	strcat(errmsg,"  |  LogMsg - ");

	makeMessage(LOG_TYPE, boolean, errmsg, errn, what);
}

void debugp(char *where, int boolean, int errn,char *what)
{
	char errmsg[21+strlen(where)];
	strcpy(errmsg,"Where - ");
	strcat(errmsg,where);
	strcat(errmsg,"  |  DebugMsg - ");

	makeMessage(DEBUG_TYPE, boolean, errmsg, errn, what);
}

int sendall(int fd, char *buf, int *len, int flags)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = send(fd, buf+total, bytesleft, flags);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    // return -1 on failure, 0 on success
    if(n > 0){
    	logp("access-sendall",0,0,"Successfully sent the complete data");
    	return 0;
    }else if(n == 0){
    	errorp("access-sendall",0,0,"Unable to send the data - Client went OFF");
    	return -1;
    }else if(n == -1){
    	errorp("access-sendall",0,0,"Unable to send the data");
    	debugp("access-sendall",1,errno,NULL);
    	return -1;
    }
}

int recvall(int fd, char *buf, int *len, int flags)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = recv(fd, buf+total, bytesleft, flags);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here
    
    // return -1 on failure, 0 on success
    if(n > 0){
    	logp("access-recvall",0,0,"Successfully recved the complete data");
    	return 0;
    }else if(n == 0){
    	errorp("access-recvall",0,0,"Unable to recv the data - Client went OFF");
    	return -1;
    }else if(n == -1){
    	errorp("access-recvall",0,0,"Unable to recv the data");
    	debugp("access-recvall",1,errno,NULL);
    	return -1;
    }
} 
