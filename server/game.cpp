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
#include "helper.h"
#include "msgQ.h"
#include <algorithm>
#include <vector> 
#include <unordered_map>
#include <sstream>

struct gameThreadArg{
    int fd;
    char plid[9];
};

unordered_map <string,pthread_t> mapThread={};
unordered_map <string, long> mapMtype= {};
vector <int> VecMtype;
pthread_mutex_t lock_mapThread;
pthread_mutex_t lock_mapMtype;
pthread_mutex_t lock_VecMtype;

int main(){
	int socket_fd, connection_fd;
	struct sockaddr_un address;
	socklen_t address_length = sizeof(address);
	int err;

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
	socket_fd = unixSocket(UNIX_SOCKET_FILE_PLA_TO_GAM, "GAME-main", LISTEN_QUEUE_SIZE);
    logp("GAME-main",0,0,"Socket made Succesfully");

    //connecting to the database
    logp("GAME-main",0,0,"Connecting to the database");
    if (ConnectDB(DATABASE_USER_NAME, DATABASE_PASSWORD, DATABASE_NAME, DATABASE_IP, DATABASE_PORT, "DISPATCHER-main") == 0){
        logp("GAME-main",0,0,"Connected to database Successfully");
    }else {
        logp("GAME-main",0,0,"Unable to connect ot the database");
    }

    //creting message queue
    createMsgQ("Game-main");

    //initializing lock
    if((err = pthread_mutex_init(&lock_mapThread,NULL)) != 0){ 
        errorp("GAME-Main",0,0,"Error initializing mutex lock for mapThread");
        debugp("GAME-Main",1,err,"");
    }
    else{
        logp("GAME-main",0,0,"Mutex lock for mapThread made Succesfully");
    }

    if((err = pthread_mutex_init(&lock_mapMtype,NULL)) != 0){ 
        errorp("GAME-Main",0,0,"Error initializing mutex lock for mapMtype");
        debugp("GAME-Main",1,err,"");
    }
    else{
        logp("GAME-main",0,0,"Mutex lock for mapMtype made Succesfully");
    }

    if((err = pthread_mutex_init(&lock_VecMtype,NULL)) !=0){ 
        errorp("GAME-Main",0,0,"Error initializing mutex lock for VecType");
        debugp("GAME-Main",1,err,"");
    }
    else{
        logp("GAME-main",0,0,"Mutex lock for Vectype made Succesfully");
    }

    //game starting for accepting connection & creating threads
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

void connection_handler(int connection_fd){
    char msgbuf[50], plid[9];
    int fd_to_recv, ret, plid_len = sizeof(plid);

    char identity[40], buf[100];
    sprintf(identity, "GAME-connection_handler-fd: %d -", connection_fd);


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
    gameThreadArg thread_arg;//structure declared at top
    thread_arg.fd = fd_to_recv;
    strcpy(thread_arg.plid, plid);
    //thread_arg_v = (void*) thread_arg;//this is not possible may be because size of struct is more than size of void pointer

    logp(identity,0,0,"Calling Calling pthread_create");
    if((err = pthread_create(&thread_id, NULL, checkThread, &thread_arg ))!=0) {
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

void* checkThread(void* arg){
    gameThreadArg playerInfo;
    playerInfo = *( (gameThreadArg*) (arg) );

    string plid, myTeam, oppTeam, gameId;
    int ret,err;
    plid.assign(playerInfo.plid);

    char identity[40], buf[100];
    sprintf(identity, "GAME-connection_handler-fd: %d -", playerInfo.fd);
    
    if((ret = getPlayerTid(plid,myTeam,identity)) == 0 )
    {
        sprintf(buf,"Player Tid - %s", myTeam);
        logp(identity,0,0,buf);
    }
    if((ret = getOppTid(myTeam,oppTeam,identity)) == 0)
    {
        sprintf(buf,"Player's opposite Tid - %s", oppTeam);
        logp(identity,0,0,buf);
    }

    gameId = myTeam + oppTeam;
    sprintf(buf,"game id - %s", gameId);
    logp(identity,0,0,buf);
//    unordered_map <string,pthread_t>::const_iterator got = mapThread.find(gameId);

//    if(got == mapThread.end()){
    if( !mapThread.count(gameId) )
        pthread_t shuffleId;

        if((err = pthread_create(&shuffleId, NULL,shuffleThread, &playerInfo))!=0) {
        errorp(identity,0,0,"Unable to create the thread");
        debugp(identity,1,err,"");
        }
        else{
            logp(identity,0,0,"created the thread successfully");
        }
        
        if((err = pthread_mutex_lock(&lock_mapThread)) !=0){
        errorp(identity,0,0,"Unable to lock the mapThread");
        debugp(identity,1,err,"");
        }
        else{
            logp(identity,0,0,"mapThread locked successfully");
        }
        mapThread[gameId] = shuffleId;

        if((err = pthread_mutex_unlock(&lock_mapThread)) !=0){
        errorp(identity,0,0,"Unable to unlock the mapThread");
        debugp(identity,1,err,"");
        }
        else{
            logp(identity,0,0,"mapThread unlocked successfully");
        }


        if((err = pthread_mutex_lock(&lock_mapMtype)) !=0){
        errorp(identity,0,0,"Unable to lock the mapMtype");
        debugp(identity,1,err,"");
        }
        else{
            logp(identity,0,0,"mapMtpype locked successfully");
        }

        if(VecMtype.empty()){
            VecMtype.push_back(1);
            mapMtype[gameId]=1;
        }
        else{
            int val=1;
            int flag =0; 
            int i;
            for(i=0;i<VecMtype.size();i++){
               if(VecMtype[i] == val)
               {
                    val++;
               }
               else
               {
                VecMtype.insert(i,val);
                flag = 1;
                break;
               }
            }
            if(flag ==1)
            {
                VecMtype.push_back(val);
            }
            mapMtype[gameId] =val; 
        }
        
        if((err = pthread_mutex_unlock(&lock)) !=0){
        errorp(identity,0,0,"Unable to unlock the lock");
        debugp(identity,1,err,"");
        }
        else{
            logp(identity,0,0,"mlock unlocked successfully");
        }


    }
    else
    {
        //message to be sent on queue
        struct playerMsg msg;
        msg.mtype = mapMtype[gameId];
        msg.plid.assign(plid);
        msg.fd = playerInfo.fd;
        msg.gameId = gameId;

        sprintf(buf,"Msg sent on queue: mtype(%d) plid(%s) fd(%d) gameId(%s) - %s", msg.mtype,msg.plid,msg.fd,msg.gameId);
        logp(identity,0,0,buf);

        msgSend(&msg,identity);
    }   

/*
    

    char plid[9];
    char name[PLAYER_NAME_SIZE], team[PLAYER_TEAM_SIZE];
    int fd;

    strncpy(plid, plid, 9);
    fd = playerInfo.fd;

    char identity[40], buf[100];
    sprintf(identity, "GAME-checkThread-fd: %d -", fd);

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
    
    //create a alarm which checks out time remaining.....
    // Sleep for 1.5 sec
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 500000;
    //select(0, NULL, NULL, NULL, &tv);
    //create the game and send the data
    */
}


char SUITS[] = {'C', 'S', 'H', 'D'};
unordered_map <char, int> RANKS = {{'A',1}, {'2',2}, {'3',3}, {'4',4}, {'5',5}, {'6',6}, {'7',7}, {'8',8}, {'9',9}, {'T',10}, {'J',11}, {'Q',12}, {'K',13} };
unordered_map <char, int> VALUES = {{'A',1}, {'2',2}, {'3',3}, {'4',4}, {'5',5}, {'6',6}, {'7',7}, {'8',8}, {'9',9}, {'T',10}, {'J',11}, {'Q',12}, {'K',13} }; 

//class Card
Card::Card(char rank, char suit, bool open = false)
    :rank(rank), suit(suit), open(open)
{}

Card::Card(){
    open = false;
}

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


//class Deck
Deck::Deck(){
    int i,j;

    for(i = 0; i < 4; i++){
        for(j = 0; j < 13; j++){
            Card c(SUITS[i], RANKS[j], false);
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

char nextPos(char pos){
    switch(pos) {
        case 'N':
            return 'E';
        case 'E':
            return 'S';
        case 'S':
            return 'W';
        case 'W':
            return 'N';
    }
}


//class Trick
Trick::Trick(char first) //N,W,E,S
    :first(first)
{
    i = 0;
}

Trick::Trick(){
    i =0;
}

string Trick::print(){
    std::ostringstream oss;
    oss << "Card1 played by '"<< first << "': " << cards[0].print() << "Card2 played by '"<< nextPos(first) << "': " << cards[1].print() << "Card3 played by '"<< nextPos(first) << "': " << cards[2].print() << "Card4 played by '"<< nextPos(first) << "': " << cards[3].print() ; 
    return oss.str();
}

void Trick::nextCard(Card c){
    cards[i++] = c;
}

Card Trick::nextCard(int i){
    return cards[i];
}

int Trick::score(){
    //TODO
}

void Trick::setWinner(){
    //TODO
}

char Trick::getWinner(){
    //TODO
}


//class Tricks
Tricks::Tricks(){
    index = 0;
}

string Tricks::print(){
    std::ostringstream oss;
    int i;
    for(i = 0; i < 13; i++){
        oss << "Trick" << i+1 << " (Winner:" << tricks[i].getWinner() << "): '" << tricks[i].print();
    }
    return oss.str();
}

void Tricks::add(Trick t){
    tricks[index++] = t;
}

Trick Tricks::get(int i){
    return tricks[i];
}

void Tricks::setWinner(){

}

char Tricks::getWinner(){

}

int Tricks::score(char team){
    int i, score = 0;
    for(i = 0; i < 13; i++){
        if(tricks[i].getWinner() == team){
            score += tricks[i].score();
        }
    }
    return score;
}


//class Player
Player::Player(string plid, char position, char team, string tid, string name, string country)
    :plid(plid), position(position), team(team), tid(tid), name(name), country(country)
{}

int Player::getUserChoice(){

}


//class Team
Team::Team(char team, string tid,string plid1, string plid2)
    :team(team), tid(tid), plid1(plid1), plid2(plid2)
{}

int Team::score(){
    
}