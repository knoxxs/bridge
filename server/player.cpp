#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>
#include <libpq-fe.h>
#include <string>
#include <iostream>
#include "access.h"
#include "psql.h"
#include "helper.h"
 
#define UNIX_SOCKET_FILE_DIS_TO_PLA "./demo_socket"
#define UNIX_SOCKET_FILE_PLA_TO_GAM "./UNIX_SOCKET_FILE_PLA_TO_GAM"
#define LISTEN_QUEUE_SIZE 5
#define IDENTITY_SIZE 40

void* playerMain(void*);
void connection_handler(int);
void contactGame(char*,int,int len);


struct playerThreadArg{
    int fd;
    char plid[9];
};

int main(){
	int socket_fd, connection_fd;
	struct sockaddr_un address;
	socklen_t address_length = sizeof(address);
	
    int logfile;
    setLogFile(STDOUT_FILENO);
    logp("PLAYER-Main", 0,0 ,"Starting");
    if( (logfile = open(LOG_PATH, O_RDWR|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)) < 0 ){
        errorp("PLAYER-Main",0,0,"Error Opening logfile");
        debugp("PLAYER-Main",1,1,"");
    }
    setLogFile(logfile);

    logp("PLAYER-main",0,0,"Starting main");

    logp("PLAYER-main",0,0,"Calling unixSocket");
	socket_fd = unixSocket(UNIX_SOCKET_FILE_DIS_TO_PLA,"PLAYER-main", LISTEN_QUEUE_SIZE);
    logp("PLAYER-main",0,0,"Socket made Succesfully");

    //connecting to the database
    logp("PLAYER-main",0,0,"Connecting to the database");
    if (ConnectDB(DATABASE_USER_NAME, DATABASE_PASSWORD, DATABASE_NAME, DATABASE_IP, DATABASE_PORT, "PLAYER-main") == 0){
        logp("PLAYER-main",0,0,"Connected to database Successfully");
    }else {
        logp("PLAYER-main",0,0,"Unable to connect ot the database");
    }

    logp("PLAYER-main",0,0,"Starting accepting connection in infinite while loop");
	while(1){
        logp("PLAYER-main",0,0,"Waiting for the connection");        
        if( (connection_fd = accept(socket_fd, (struct sockaddr *) &address,&address_length)) > 0){
    		logp("PLAYER-main",0,0,"Connection recvd Succesfully and calling connection_handler");
            connection_handler(connection_fd);
    		close(connection_fd);
        }else{
            errorp("PLAYER-Main",0,0,"Error accepting connection");
            debugp("PLAYER-Main",1,errno,"");
        }
	}

    //TODO graceful exit
    logp("PLAYER-main",0,0,"Closing the connection with the database");
    CloseConn("PLAYER-main");
}


void connection_handler(int connection_fd){
    char msgbuf[50], plid[9];
    int fd_to_recv, ret, plid_len = sizeof(plid);

    char identity[40], buf[100];
    sprintf(identity, "PLAYER-connection_handler-fd: %d -", connection_fd);


    //recieving fd of the player
    logp(identity,0,0,"Calling recv_fd");
    fd_to_recv = recv_fd(connection_fd ,&errcheckfunc, plid, sizeof(plid), identity);
    logp(identity,0,0,"fd recvd successfuly");

    //recieving plid of the player
    logp(identity,0,0,"recieving the plid fo the player");
    if((ret = recvall(connection_fd, plid, &plid_len, 0)) != 0) {
        errorp(identity,0,0,"Unable to recv the plid");
        debugp(identity,1,errno,"");
    }
    
    sprintf(buf,"recvd fd is (%d) and plid is (%s)",fd_to_recv, plid);
    logp(identity,0,0,buf);

    //checking if the fd we have recieved is correct or not
    logp(identity,0,0,"recieving the checking data");
    if((ret = recv(fd_to_recv, msgbuf, 6, 0)) == -1) {
        errorp(identity,0,0,"Unable to recv the checking data");
        debugp(identity,1,errno,"");
    }
    msgbuf[6] ='\0';
    sprintf(buf,"Checking data recvd is - %s", msgbuf);
    logp(identity,0,0,buf);
    
    //creating thread of the player
    pthread_t thread_id = 0;
    //void* thread_arg_v;
    int err;

    logp(identity,0,0,"Creating the thread argument");
    playerThreadArg thread_arg;//structure declared at top
    thread_arg.fd = fd_to_recv;
    strcpy(thread_arg.plid, plid);
    //thread_arg_v = (void*) thread_arg;//this is not possible may be because size of struct is more than size of void pointer

    logp(identity,0,0,"Calling Calling pthread_create");
    if((err = pthread_create(&thread_id, NULL, playerMain, &thread_arg ))!=0) {
        errorp(identity,0,0,"Unable to create the thread");
        debugp(identity,1,err,"");
    }

    logp(identity,0,0,"Calling pthread_detach");
    if ((err = pthread_detach(thread_id)) != 0) {
        errorp(identity,0,0,"Unable to detach the thread");
        debugp(identity,1,err,"");
    }

    logp(identity,0,0,"Closing fd_to_recv");
    close(fd_to_recv);

    return;
}

void* playerMain(void* arg){
    playerThreadArg playerInfo;
    playerInfo = *( (playerThreadArg*) (arg) );

    char plid[9];
    char name[PLAYER_NAME_SIZE], team[PLAYER_TEAM_SIZE];
    int fd;

    strncpy(plid, playerInfo.plid, 9);
    fd = playerInfo.fd;

    char identity[40], buf[100];
    sprintf(identity, "PLAYER-playerMain-fd: %d -", fd);

    logp(identity,0,0,"New thread created Succesfully");

    sprintf(buf, "Calling get player info with plid %s", plid);
    logp(identity,0,0,buf);
    if(getPlayerInfo(plid , name, team, fd, sizeof(plid), sizeof(name), sizeof(team), identity) == 0 ){
        sprintf(buf,"This is player info id(%s) name(%s) team(%s)\n", plid, name, team);
        logp(identity,0,0,buf);
    }else{
        logp(identity,0,0,"Unable to retrieve the player info, exiting from this thread");
        exit(-1);
    }

    //////////////////////_____creating schedule alarm_____///////////////
    
    //check for latest match
    string timestamp;
    if( getPlayerSchedule(plid , timestamp, identity) == 0){
        sprintf(buf,"This is player next match timestamp (%s)\n", timestamp.c_str());
        logp(identity,0,0,buf);
    }else{
        logp(identity,0,0,"Unable to retrieve next match timestamp, exiting from this thread");
        exit(-1);
    }
    
    // getting system time in a string
    time_t now;
    char * dat;
    string cur_time;
    struct timeval tv;
    tv.tv_sec = 0;
    int ret;
   // time(&now);
    //dat = ctime(&now);
    //dat = strtok(dat,"\n");
    //cur_time.assign(dat);

    timestamp = "2012-12-29 01:20:55";
    Diff_Time td(timestamp);
   
    do
    {
        //sprintf(buf,"tv_tsec(%d)\n", tv.tv_sec);
        //logp(identity,0,0,buf);
        if((ret = select(0, NULL, NULL, NULL, &tv))== -1)
        {
             errorp(identity,0,0,"select not working");
             debugp(identity,1,errno,"");
        }

        //select(0, NULL, NULL, NULL, &tv);
        time(&now);
        dat = ctime(&now);
        dat = strtok(dat,"\n");
        cur_time.assign(dat);
        sprintf(buf,"current time(%s)\n", cur_time.c_str());
        logp(identity,0,0,buf);
        string change_cur_time = date_conv(cur_time);
        sprintf(buf,"change current time(%s)\n", change_cur_time.c_str());
        logp(identity,0,0,buf);
        td.setParam(2,change_cur_time);

        if(td.getHour() >= 5)
        {
            logp(identity,0,0,"More than 5 hours left, come after some time");
            //TODO
        }
        else
        if(td.getHour() >= 1)
        {
            sprintf(buf,"Time remaining in hours(%d)\n", td.getHour());
            logp(identity,0,0,buf);
            tv.tv_sec = 1800;
        }
        else
        if(td.getMin() >= 15)
        {
            sprintf(buf,"Time remaining in minutes(%d)\n", td.getMin());
            logp(identity,0,0,buf);
            tv.tv_sec = 600;
       
        }
        else
        if(td.getMin() >= 5)    
        {
            sprintf(buf,"Time remaining in minutes(%d)\n", td.getMin());
            logp(identity,0,0,buf);
            tv.tv_sec = 180;
        }
        else
        {
            sprintf(buf,"Time remaining in minutes(%d)\n", td.getMin());
            logp(identity,0,0,buf);
            tv.tv_sec = 0;
        }
    }while(tv.tv_sec > 0);



    //create a alarm which checks out time remaining.....
    // Sleep for 1.5 sec
    
    //tv.tv_sec = 1;
    //tv.tv_usec = 500000;
    //select(0, NULL, NULL, NULL, &tv);
    //create the game and send the data

    
     

    sprintf(buf,"Player id is %s and Contacting Game",plid);
    logp(identity,0,0,buf);
    contactGame( plid, fd, sizeof(plid));
    logp(identity,0,0,"Contacted To Game succesfully");


}

void contactGame(char* plid, int fd_to_send, int len){
    int socket_fd;

    char identity[IDENTITY_SIZE], buf[100];
    sprintf(identity, "PLAYER-contact-Player-fd: %d -", fd_to_send);    

    logp(identity,0,0,"Inside contactPlayer and calling unixClientSocket");
    sprintf(buf,"PLAYER-unixClientSocket-fd: %d -",fd_to_send);
    socket_fd = unixClientSocket(UNIX_SOCKET_FILE_PLA_TO_GAM,buf,sizeof(buf));
    sprintf(buf,"Recved socket-fd: %d -",socket_fd);
    logp(identity,0,0,buf);

    logp(identity,0,0,"Calling send_fd");
    send_fd(socket_fd, fd_to_send, plid, len, identity);
    logp(identity,0,0,"fd sent Successfully");

    sprintf(buf, "Sending plid to game process ,plid_length(%d)",len);
    logp(identity,0,0,buf);
    if( sendall(socket_fd, plid, &len, 0) != 0){
        errorp(identity, 0, 0, "Unable to send complete plid");
        debugp(identity,1,errno,"");
    }
}

