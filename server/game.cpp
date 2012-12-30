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

struct shuffleToGameThread{

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

    sleep(1);

    return;
}

void shuffleThread(void* arg){
    playerMsg playerInfo;
    playerInfo = *( (playerMsg*) (arg) );

    string gameId, plid, teamId, name;
    long mtype;
    int fd, playerRecvd = 1;
    char subTeamId, pos;

    mtype = playerInfo.mtype;
    plid = playerInfo.plid;
    fd = playerInfo.fd;
    gameId = playerInfo.gameId;
    subTeamId = playerInfo.subTeamId;

    char identity[40], buf[100], nameTemp[PLAYER_NAME_SIZE], team[PLAYER_TEAM_SIZE];
    sprintf(identity, "GAME-shuffleThread-gameId: %s -", gameId.c_str());

    logp(identity,0,0,"Inside shuffleThread");
    sprintf(buf,"Player1 recvd: mtype(%ld), plid(%s), fd(%d), gameId(%s), subTeamId(%c)",mtype, plid.c_str(), fd, gameId.c_str(), subTeamId);
    logp(identity,0,0,buf);

    if(getPlayerInfo(plid.c_str(), nameTemp, team, fd, plid.length() + 1, sizeof(nameTemp), sizeof(team), identity) == 0 ){
        sprintf(buf,"This is player info id(%s) name(%s) team(%s)\n", plid.c_str(), nameTemp, team);
        logp(identity,0,0,buf);
    }else{
        logp(identity,0,0,"Unable to retrieve the player info, exiting from this thread");
    }
    teamId.assign(team);
    name.assign(nameTemp);

    Game gameA(gameId, 'A'), gameB(gameId, 'B');

    Player player(plid, 'N', subTeamId, teamId, name, fd);

    bool ansPos = false, aewPos = false, bnsPos = false, bewPos = false;

    if(player.subTeamId == 'A'){
        gameA.setPlayer(player, 'N');
        ansPos = true;
    }else{
        gameB.setPlayer(player, 'N');
        bnsPos = true;
    }

    while( playerRecvd < 8 ){
        logp(identity,0,0,"Inside recving while loop");
        if(msgRecv(&playerInfo, mtype, identity) != 0){
            errorp(identity,0,0,"Unable to recv the msg");
        }else{
            plid = playerInfo.plid;
            fd = playerInfo.fd;
            subTeamId = playerInfo.subTeamId;

            if(getPlayerInfo(plid.c_str(), nameTemp, team, fd, plid.length() + 1, sizeof(nameTemp), sizeof(team), identity) == 0 ){
                sprintf(buf,"This is player info id(%s) name(%s) team(%s)\n", plid.c_str(), nameTemp, team);
                logp(identity,0,0,buf);
            }else{
                logp(identity,0,0,"Unable to retrieve the player info, exiting from this thread");
            }
            teamId.assign(team);
            name.assign(nameTemp);

            if(subTeamId == 'A'){
                if(ansPos){
                    if(gameA.N.tid == teamId){
                        pos = 'S';
                    }else if(aewPos){
                        pos = 'W';
                    }else{
                        pos = 'E';
                        aewPos = true;
                    }
                }else{
                    pos = 'N';
                    ansPos = true;
                }
            }else{
                if(bnsPos){
                    if(gameB.N.tid == teamId){
                        pos = 'S';
                    }else if(bewPos){
                        pos = 'W';
                    }else{
                        pos = 'E';
                        bewPos = true;
                    }
                }else{
                    pos = 'N';
                    bnsPos = true;
                }

            }
            Player player(plid, pos, subTeamId, teamId, name, fd);

            if(player.subTeamId == 'A'){
                gameA.setPlayer(player, pos);
            }else{
                gameB.setPlayer(player, pos);
            }

            playerRecvd++;
            sprintf(buf,"Player%d recvd: mtype(%ld), plid(%s), fd(%d), gameId(%s), subTeamId(%c)",playerRecvd, playerInfo.mtype, playerInfo.plid.c_str(), playerInfo.fd, playerInfo.gameId.c_str(), subTeamId);
            logp(identity,0,0,buf);
        }
    }
    logp(identity,0,0,"Recvd all the players");

    Deck deck();
    deck.shuffle();

    for(int i = 0, i < 52 ; i += 4){
        gameA.N.addCard(deck.deal());
        gameA.E.addCard(deck.deal());
        gameA.S.addCard(deck.deal());
        gameA.W.addCard(deck.deal());
    }

    gameB.N.addCard(gameA.N.cards);
    gameB.E.addCard(gameA.E.cards);
    gameB.S.addCard(gameA.S.cards);
    gameB.W.addCard(gameA.W.cards);
    return;
}

void* checkThread(void* arg){
    gameThreadArg playerInfo;
    playerInfo = *( (gameThreadArg*) (arg) );

    int val=1, ret, fd;

    string plid, myTeam, oppTeam, gameId;
    plid.assign(playerInfo.plid);
    fd = playerInfo.fd;

    char identity[40], buf[100], subTeamId;
    sprintf(identity, "GAME-checkThread-fd: %d -", fd);
    
    logp(identity,0,0,"Calling getPlayerTeamInfo");
    if((ret = getPlayerTeamInfo(plid,myTeam, &subTeamId, identity)) == 0 ){
        sprintf(buf,"Player Tid - %s", myTeam.c_str());
        logp(identity,0,0,buf);
    }else{
        sprintf(buf,"error returned from getPlayerTeamInfo ,ret val - %d", ret);
        errorp(identity,0,0,buf);
    }

    logp(identity,0,0,"Calling getOppTid");
    if((ret = getOppTid(myTeam,oppTeam,identity)) == 0){
        sprintf(buf,"Player's opposite Tid - %s", oppTeam.c_str());
        logp(identity,0,0,buf);
    }else{
        sprintf(buf,"error returned from getOppTid ,ret val - %d", ret);
        errorp(identity,0,0,buf);
    }

    gameId = myTeam + oppTeam;
    sprintf(buf,"game id - %s", gameId.c_str());
    logp(identity,0,0,buf);

    playerMsg msg;
    msg.mtype = -1;
    msg.plid = plid;
    msg.fd = fd;
    msg.gameId = gameId;
    msg.subTeamId = subTeamId;

    if( !mapThread.count(gameId) ){
        pthread_t shuffleId;

        mutexLock(&lock_mapThread, identity, "mapThread");

        logp(identity,0,0,"Calling Calling pthread_create");
        if((ret = pthread_create(&shuffleId, NULL, shuffleThread, &msg))!=0) {
            errorp(identity,0,0,"Unable to create the thread");
            debugp(identity,1,ret,"");
        }

        logp(identity,0,0,"Calling pthread_detach");
        if ((ret = pthread_detach(shuffleId)) != 0) {
            errorp(identity,0,0,"Unable to detach the thread");
            debugp(identity,1,ret,"");
        }
        
        mapThread[gameId] = shuffleId;
        mutexUnlock(&lock_mapThread, identity, "mapThread");

        mutexLock(&lock_VecMtype, identity, "VecMtype");
        for(vector<int>::iterator it = VecMtype.begin(); it != VecMtype.end(); ++it) {
            if(*it == val){

                val++;
            }
            else{
                break;
            }
        }
        VecMtype.insert(it,val);
        mutexUnlock(&lock_VecMtype, identity, "VecMtype");

        mutexLock(&lock_mapMtype, identity, "mapMtype");
        mapMtype[gameId] =val;
        mutexLock(&lock_mapMtype, identity, "mapMtype");

        sleep(1);
    }
    else{
        msg.mtype = val;

        sprintf(buf,"Sending msg to queue: mtype(%d) plid(%s) fd(%d) gameId(%s) - %s", msg.mtype, msg.plid.c_str(), msg.fd, msg.gameId.c_str());
        logp(identity,0,0,buf);
        if(msgSend(&msg, identity) != 0){
            errorp(identity,0,0,"Unable to send the msg");
        }
        logp(identity,0,0,"Message sent successfuly, exiting the thread");
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
Player::Player(string plid, char position, char team, string tid, string name, int fd)
    :plid(plid), position(position), subTeamId(team), tid(tid), name(name), fd(fd)
{
    i = 0;
}

void Player::addCard(Card c){
    cards[i++] = c;
}

void Player::addCard(vector <Card> &crds){
    cards = crds;
}

int Player::getUserChoice(){

}


//class Team
Team::Team(char team, string tid,string plid1, string plid2)
    :team(team), tid(tid), plid1(plid1), plid2(plid2)
{}

int Team::score(){
    
}


//class game
Game::Game(string gid, char stid)
    :gameId(gid), subTeamId(stid)
{}
Game::Game(string gid, char stid, Player& n,Player& s, Player& e, Player& w)
    :gameId(gid), subTeamId(stid), N(n), S(s), E(e),W(s)
{}
void Game::setPlayer(Player& p, char pos){
    switch(pos) {
        case 'N':
            N = p;
        case 'E':
            E = p;
        case 'S':
            S = p;
        case 'W':
            W = p;
    }    
}
