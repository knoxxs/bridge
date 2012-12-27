#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include "access.h"
#include <string>
#include <iostream>
#include <sstream>
int logfile;

void setLogFile(int logfd){
	logfile = logfd;
}

void logWrite(int typ, string msg) // typ --> type(category) of message [1-Normal Log, 2-Warning(any unexpected thing happened), 3-Error, 4-Debugging Log ]
{
	int fd;
	time_t now;
	ssize_t wlength=0;
	char * dat;
//	int size = 45+strlen(msg);//14+24+5+sizeof msg+1
	
//	str= (char *) malloc(size);
	
	time(&now);//system time in seconds
	dat = ctime(&now); // converting seconds to date-time format
	dat = strtok(dat,"\n");
	
	string date,str;

	date.assign(dat);
	//Appending type of log
	switch(typ)
	{
	case LOG_TYPE:
		str = "__LOG__    |  ";
		str = str + dat;
		break;
	case WARN_TYPE:
		str = "__WARN__   |  ";
		str = str + dat;
		break;
	case ERROR_TYPE:
		str = "__ERR__    |  ";
		str = str + dat;
		break;
	case DEBUG_TYPE:
		str = "__DEBUG__  |  ";
		str = str + dat;
		break;
	default:
		str = "__UNDEF__  |  ";
		str = str + dat;
		break;
	}
	
	
	str = str + "  |  ";
	str = str + msg;//appending message
	str = str + "\n";
	
	//fd = open("log", O_WRONLY | O_CREAT | O_APPEND, 0644); // should be opened somewhere else
	fd = logfile;
	if (fd == -1)
		printf("Could not open log - %s\n",strerror(errno));
	else
	{//need to add lock to the file and printing error message
		while ( wlength < str.length() )
		{
			wlength = write(fd, str.c_str(),str.length());
			if (wlength == -1)
			{
				printf("Error : writing log\n");
				break;
			}
		}
	}
}


void makeMessage(int type, int boolean, string errmsg, int errn, string what){
	if(boolean == 1) { //we got error number
		errmsg = errmsg + strerror(errn);
		//fprintf(stderr,"ERROR - In %s and error is %s\n",where ,strerror(errn));
		logWrite(type,errmsg);
	}
	else if(boolean == 0){ //we got a message
		errmsg = errmsg + what;
		//fprintf(stderr,"ERROR - In %s and error is %s\n",where ,what);
		logWrite(type,errmsg);
	}
	else{ //we got nothing
		errmsg = errmsg + "No Message";
		//fprintf(stderr,"ERROR - In %s\n",where);
		logWrite(type,errmsg);	
	}
} 

void errorp(string where, int boolean, int errn,string what)
{
	string errmsg;
	errmsg = "Where - ";
	errmsg = errmsg + where;
	errmsg = errmsg + "  |  Error - ";

	makeMessage(ERROR_TYPE, boolean, errmsg, errn, what);
}

void logp(string where, int boolean, int errn,string what)
{
	string errmsg;
	errmsg = "Where - ";
	errmsg = errmsg + where;
	errmsg = errmsg + "  |  LogMsg - ";

	makeMessage(LOG_TYPE, boolean, errmsg, errn, what);
}

void debugp(string where, int boolean, int errn,string what)
{
	string errmsg;
	errmsg = "Where - ";
	errmsg = errmsg +  where;
	errmsg = errmsg + "  |  DebugMsg - ";

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
    	printf("%s\n","thi is it" );
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


bool str2int (int &i, string s)
{
    char c;
    std::stringstream ss(s);
    ss >> i;
    if (ss.fail() || ss.get(c)) {
        // not an integer
        return false;
    }
    return true;
}

//***********************Time Functions defined**************************

Time::Time(string s)
{
	str2int(year,s.substr(0,4));
	str2int(month,s.substr(5,2));
	str2int(date,s.substr(8,2));
	str2int(hour,s.substr(11,2));
	str2int(min,s.substr(14,2));
	str2int(sec,s.substr(17,2));
}

int Time::getYear()
{
	return year;
}

int Time::getMonth()
{
	return month;
}
int Time::getDate()
{
	return date;
}
int Time::getHour()
{
	return hour;
}
int Time::getMin()
{
	return min;
}
int Time::getSec()
{
	return sec;
}
