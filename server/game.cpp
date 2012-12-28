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
#include "game.h"
#include <algorithm>
#include <vector> 
#include <unordered_map>
#include <sstream>

/* size of control buffer to send/recv one file descriptor */
#define CONTROLLEN  CMSG_LEN(sizeof(int))
#define PLAYER_NAME_SIZE 31 //30 + 1
#define PLAYER_TEAM_SIZE 9 //8+1
#define DATABASE_USER_NAME "postgres"
#define DATABASE_PASSWORD "123321"
#define DATABASE_NAME "bridge"
#define DATABASE_IP "127.0.0.1"
#define DATABASE_PORT "5432"
#define MAXLINE 2
#define UNIX_SOCKET_FILE_PLA_TO_GAM "./UNIX_SOCKET_FILE_PLA_TO_GAM"
#define LOG_PATH "./log"
#define LISTEN_QUEUE_SIZE 10

static struct cmsghdr   *cmptr = NULL;      /* malloc'ed first time */

int unixSocket();
int recv_fd(int , ssize_t (*userfunc)(int, const void *, size_t), char*, int len);
ssize_t errcheckfunc(int, const void *, size_t);
void* gameMain(void*);
void connection_handler(int);
int getPlayerInfo(char *, char *, char *, int, int, int, int);


struct gameThreadArg{
    int fd;
    char plid[9];
};

int main(){
	int socket_fd, connection_fd;
	struct sockaddr_un address;
	socklen_t address_length = sizeof(address);
	
    int logfile;
    setLogFile(STDOUT_FILENO);
    logp("GAME-Main", 0,0 ,"Starting");
    if( (logfile = open(LOG_PATH, O_RDWR|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)) < 0 ){
        errorp("GAME-Main",0,0,"Error Opening logfile");
        debugp("GAME-Main",1,1,"");
    }
    setLogFile(logfile);

    logp("GAME-main",0,0,"Starting main");

    logp("GAME-main",0,0,"Calling unixSocket");
	socket_fd = unixSocket();
    logp("GAME-main",0,0,"Socket made Succesfully");

    //connecting to the database
    logp("GAME-main",0,0,"Connecting to the database");
    if (ConnectDB(DATABASE_USER_NAME, DATABASE_PASSWORD, DATABASE_NAME, DATABASE_IP, DATABASE_PORT, "DISPATCHER-main") == 0){
        logp("GAME-main",0,0,"Connected to database Successfully");
    }else {
        logp("GAME-main",0,0,"Unable to connect ot the database");
    }

    logp("GAME-main",0,0,"Starting accepting connection in infinite while loop");
	while(1){
        logp("GAME-main",0,0,"Waiting for the connection");        
        if( (connection_fd = accept(socket_fd, (struct sockaddr *) &address,&address_length)) > 0){
    		logp("GAME-main",0,0,"Connection recvd Succesfully and calling connection_handler");
            connection_handler(connection_fd);
    		close(connection_fd);
        }else{
            errorp("GAME-Main",0,0,"Error accepting connection");
            debugp("GAME-Main",1,errno,"");
        }
	}

    //TODO graceful exit
    logp("GAME-main",0,0,"Closing the connection with the database");
    CloseConn("GAME-main");
}

int unixSocket(){
    struct sockaddr_un address;
    int socket_fd;
    socklen_t address_length;
    char buf[100];

    logp("GAME-unixSocket",0,0,"Inside unixSocket and calling socket");
    if( (socket_fd = socket(PF_UNIX, SOCK_STREAM, 0) ) < 0 ) {
        errorp("GAME-unixSocket",0,0,"Unable to create the socket");
        debugp("GAME-unixSocket",1,errno,"");
        return -1;
    } 


    unlink(UNIX_SOCKET_FILE_PLA_TO_GAM);

    /* start with a clean address structure */
    memset(&address, 0, sizeof(struct sockaddr_un));

    logp("GAME-unixSocket",0,0,"Making struct");
    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, sizeof(address.sun_path)-1, UNIX_SOCKET_FILE_PLA_TO_GAM);

    logp("GAME-unixSocket",0,0,"Calling bind");
    if(bind(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) != 0){
        errorp("GAME-unixSocket",0,0,"Unable to bind to the socket");
        debugp("GAME-unixSocket",1,errno,"");
        return -1;
    }

    logp("GAME-unixSocket",0,0,"calling listen");
    if(listen(socket_fd,LISTEN_QUEUE_SIZE ) != 0) {
        errorp("GAME-unixSocket",0,0,"Unable to create the socket");
        debugp("GAME-unixSocket",1,errno,"");
        return -1;
    }

    sprintf(buf,"returning socket_fd %d",socket_fd);
    logp("GAME-unixSocket",0,0,buf);
    return socket_fd;
}

void connection_handler(int connection_fd){
    char msgbuf[50], plid[9];
    int fd_to_recv, ret, plid_len = sizeof(plid);

    char identity[40], buf[100];
    sprintf(identity, "GAME-connection_handler-fd: %d -", connection_fd);


    //recieving fd of the player
    logp(identity,0,0,"Calling recv_fd");
    fd_to_recv = recv_fd(connection_fd ,&errcheckfunc, plid, sizeof(plid));
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
    gameThreadArg thread_arg;//structure declared at top
    thread_arg.fd = fd_to_recv;
    strcpy(thread_arg.plid, plid);
    //thread_arg_v = (void*) thread_arg;//this is not possible may be because size of struct is more than size of void pointer

    logp(identity,0,0,"Calling Calling pthread_create");
    if((err = pthread_create(&thread_id, NULL, gameMain, &thread_arg ))!=0) {
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

void* gameMain(void* arg){
    gameThreadArg playerInfo;
    playerInfo = *( (gameThreadArg*) (arg) );

    char plid[9];
    char name[PLAYER_NAME_SIZE], team[PLAYER_TEAM_SIZE];
    int fd;

    strncpy(plid, playerInfo.plid, 9);
    fd = playerInfo.fd;

    char identity[40], buf[100];
    sprintf(identity, "GAME-gameMain-fd: %d -", fd);

    logp(identity,0,0,"New thread created Succesfully");

    sprintf(buf, "Calling get player info with plid %s", plid);
    logp(identity,0,0,buf);
    if(getPlayerInfo(plid , name, team, fd, sizeof(plid), sizeof(name), sizeof(team)) == 0 ){
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
    
    //create a alarm which checks out time remaining.....
    // Sleep for 1.5 sec
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 500000;
    //select(0, NULL, NULL, NULL, &tv);
    //create the game and send the data
}

int getPlayerInfo(char *plid, char *name, char *team, int identity_fd,int p_len, int n_len, int t_len){
    int ret;
    char identity[40], buf[100];
    sprintf(identity, "GAME-getPlayerInfo-fd: %d -", identity_fd);
    
    sprintf(buf, "Connected to data Succesfully and calling getPlayerInfoFromDb with plid %s",plid);
    logp(identity,0,0,buf);
    ret = getPlayerInfoFromDb(plid, name, team, identity);
    logp(identity,0,0,"Returned from getPlayerInfoFromDb");

    sprintf(buf,"Returning from getPlayerInfo with ret value (%d)",ret);
    logp(identity,0,0,buf);
    return ret;// 0 - done, -1 - error in getPlayerInfoFromDb
}

int recv_fd(int fd, ssize_t (*userfunc)(int, const void *, size_t), char *plid, int len) {
    int             newfd, nr, status;
    char            *ptr;
    char            buf[MAXLINE];
    //struct iovec    iov[2];
    struct iovec    iov[1];
    struct msghdr   msg;

    char identity[40], tempbuf[100];
    sprintf(identity, "GAME-recv_fd-fd: %d -", fd);

    logp(identity,0,0,"Entering the infite for loop");
    status = -1;
    for ( ; ; ) 
    {
        logp(identity,0,0,"Stuffing iovec");
        iov[0].iov_base = buf;
        iov[0].iov_len  = sizeof(buf);
        //iov[1].iov_base = plid;
        //iov[1].iov_len  = 8;

        logp(identity,0,0,"Filling msghdr");
        msg.msg_iov     = iov;
        //msg.msg_iovlen  = 2;
        msg.msg_iovlen  = 1;
        msg.msg_name    = NULL;
        msg.msg_namelen = 0;

        logp(identity,0,0,"Allocating memory");
        if (cmptr == NULL && (cmptr = malloc(CONTROLLEN)) == NULL)
            return(-1);

        logp(identity,0,0,"Assinging allocated memory to msghdr");
        msg.msg_control    = cmptr;

        sprintf(tempbuf,"Adding lenght of allocated memory, lenght - controllen(%d)",CONTROLLEN);
        logp(identity,0,0,tempbuf);
        msg.msg_controllen = CONTROLLEN;
        
        logp(identity,0,0,"Calling recvmsg");
        nr = recvmsg(fd, &msg, 0);
        if (nr < 0) {
            errorp(identity,0,0,"Unable to recv the msg");
            debugp(identity,1,errno,"");
            return -1;
        } else if (nr == 0) {
            logp(identity,0,0,"Unable to recv the msg becz connection is closed by client");
            return -1;
        }
        /*
        * See if this is the final data with null & status.  Null
        * is next to last byte of buffer; status byte is last byte.
        * Zero status means there is a file descriptor to receive.
        */
        logp(identity,0,0,"Entering the data traversing for loop");
        for (ptr = buf; ptr < &buf[nr]; ) 
        {
            logp(identity,0,0,"Inside traversing for loop");
            if (*ptr++ == 0) 
            {
                logp(identity,0,0,"Recvd First zero");
                if (ptr != &buf[nr-1])
                    errorp(identity,0,0,"Message Format error");
                status = *ptr & 0xFF;  /* prevent sign extension */
                if (status == 0) {
                    sprintf(tempbuf,"msg_controllen recvd is(%d) and must be is(%d)\n",msg.msg_controllen, CONTROLLEN );
                    logp(identity,0,0,tempbuf);
                    if (msg.msg_controllen != CONTROLLEN)
                        logp(identity,0,0,"Status =0 but no fd");
                    newfd = *(int *)CMSG_DATA(cmptr);
                    sprintf(tempbuf, "New fd extracted is %d",newfd);
                    logp(identity,0,0,tempbuf);
                } else {
                    logp(identity,0,0,"assigning newfd = error status");
                    newfd = -status;
                }
                nr -= 2;
            }
        }

        logp(identity,0,0,"Checking whether fd recvd succesfully or not");
        if (nr > 0 && (*userfunc)(STDERR_FILENO, buf, nr) != nr)
            logp(identity,0,0,"Error extracting error from the msg");
            //return(-1);
        if (status >= 0){    /* final data has arrived */
            logp(identity,0,0,"Returning new fd");
            return(newfd);  /* descriptor, or -status */
        }
    }
}

ssize_t errcheckfunc(int a,const void *b, size_t c){
    logp("GAME-errcheckfunc",0,0,"Inside error check function");
    return 0;
}

char SUITS[] = {'C', 'S', 'H', 'D'};
unordered_map <char, int> RANKS = {{'A',1}, {'2',2}, {'3',3}, {'4',4}, {'5',5}, {'6',6}, {'7',7}, {'8',8}, {'9',9}, {'T',10}, {'J',11}, {'Q',12}, {'K',13} };
unordered_map <char, int> VALUES = {{'A',1}, {'2',2}, {'3',3}, {'4',4}, {'5',5}, {'6',6}, {'7',7}, {'8',8}, {'9',9}, {'T',10}, {'J',11}, {'Q',12}, {'K',13} }; 


Card::Card(char rank, char suit)
    :rank(rank), suit(suit)
{}

string Card::print(){
    std::ostringstream oss;
    oss << "Card's Rank: '" << rank << "' and Suit: '"<< suit <<"'" <<endl;
    return oss.str();
}

char Card::getRank(){
    return rank;
}

char Card::getSuit(){
    return suit;
}


Deck::Deck(){
    int i,j;

    for(i = 0; i < 4; i++){
        for(j = 0; j < 13; j++){
            Card c(SUITS[i], RANKS[j]);
            deck.push_back(c);
        }
    } 
}

void Deck::shuffle(){
   random_shuffle(deck.begin(), deck.end());
}

Card Deck::deal(){
    Card c = deck.back();
    deck.pop_back();
    return c;
}