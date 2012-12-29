#include "msgQ.h"
#include "access.h"
#include <string>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

int msgQueId;

void createMsgQ(char * identity){
	char cmpltIdentity[CMPLT_IDENTITY_SIZE], buf[150];
	strcpy(cmpltIdentity, identity);
	strcat(cmpltIdentity,"-createMsgQ");

	logp(cmpltIdentity, 0,0, "Inside createMsgQ");

	int QId;
	logp(cmpltIdentity, 0,0, "Calling msgget");
	if((QId = msgget(MSGQKEY, MSGQFLAG )) < 0) {
    	errorp(cmpltIdentity, 0,0, "Unable to create the msgQ");
    	debugp(cmpltIdentity, 1,errno, "");
    }
    sprintf(buf,"MsgQ created successfully, ID: %d",QId);
    logp(cmpltIdentity, 0,0, buf);
    msgQueId = QId;
}

int msgSend(playerMsg* msg, char *identity){
	char cmpltIdentity[CMPLT_IDENTITY_SIZE], buf[150];
	strcpy(cmpltIdentity, identity);
	strcat(cmpltIdentity,"-msgSend");

	logp(cmpltIdentity, 0,0, "Inside msgSend");

	size_t size = sizeof(playerMsg) - sizeof(long);
	logp(cmpltIdentity, 0,0, "Calling msgsnd");
	if(msgsnd(msgQueId, msg, size, MSGSENDFLAG) < 0) {
		errorp(cmpltIdentity, 0,0, "Unable to send the message");
	  	debugp(cmpltIdentity, 1,errno, "");
	  	return -1;
	}
	logp(cmpltIdentity, 0,0, "msg sent successfully");

	return 0;
}

int msgRecv(playerMsg* msg, int msgType, char *identity){
	char cmpltIdentity[CMPLT_IDENTITY_SIZE], buf[150];
	strcpy(cmpltIdentity, identity);
	strcat(cmpltIdentity,"-msgRecv");

	logp(cmpltIdentity, 0,0, "Inside msgRecv");

	size_t size = sizeof(playerMsg) - sizeof(long);
	logp(cmpltIdentity, 0,0, "Calling msgrcv");
	if(msgrcv(msgQueId, msg, size, msgType, MSGRECVFLAG) < 0) {
		errorp(cmpltIdentity, 0,0, "Unable to recv the message");
	  	debugp(cmpltIdentity, 1,errno, "");
	  	return -1;
	}
	logp(cmpltIdentity, 0,0, "msg recved successfully");

	return 0;
}
