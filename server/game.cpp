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
#include "jsoncpp/json.h"

struct gameThreadArg{
    int fd;
    char plid[9];
};

struct shuffleToGameThread
{
    Game* game;
};

char SUITS[] = {'C', 'D', 'H', 'S'};
char RANKS[] = {'A', '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K'};
unordered_map <char, int> VALUES = {{'A',1}, {'2',2}, {'3',3}, {'4',4}, {'5',5}, {'6',6}, {'7',7}, {'8',8}, {'9',9}, {'T',10}, {'J',11}, {'Q',12}, {'K',13} }; 

unordered_map <string, int> commandsDataLen= {{"CARDS",363}, {"CARDO", 57}, {"CARDM", 57}, {"BIDOT", 31}, {"BIDMY", 31}};

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

    char buf[100];
    int logfile;
    setLogFile(STDOUT_FILENO);
    logp("GAME-Main", 0,0 ,"Starting");
    if( (logfile = open(LOG_PATH, O_RDWR|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)) < 0 ){
        errorp("GAME-Main",0,0,"Error Opening logfile");
        debugp("GAME-Main",1,1,"");
    }
    setLogFile(logfile);

    logp("GAME-main",0,0,"Starting main");

    strcpy(buf, "GAME-main");
    logp("GAME-main",0,0,"Calling unixSocket");
	socket_fd = unixSocket(UNIX_SOCKET_FILE_PLA_TO_GAM, buf, LISTEN_QUEUE_SIZE);
    logp("GAME-main",0,0,"Socket made Succesfully");

    //connecting to the database
    logp("GAME-main",0,0,"Connecting to the database");
    if (ConnectDB(DATABASE_USER_NAME, DATABASE_PASSWORD, DATABASE_NAME, DATABASE_IP, DATABASE_PORT, buf) == 0){
        logp("GAME-main",0,0,"Connected to database Successfully");
    }else {
        logp("GAME-main",0,0,"Unable to connect ot the database");
    }

    //creting message queue
    createMsgQ(buf);

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

void gameThread(void* arg)
{
    shuffleToGameThread gameInfo;
    gameInfo = *((shuffleToGameThread*) (arg));

    Game game(gameInfo.game->gameId, gameInfo.game->subTeamId);   // copy constructor where v r passing a pointer
    game.players[0] = gameInfo.game->players[0];
    game.players[1] = gameInfo.game->players[1];
    game.players[2] = gameInfo.game->players[2];
    game.players[3] = gameInfo.game->players[3];

    int ret;
    char identity[40], buf[150];
    sprintf(identity, "GAME-shuffleThread-gameId: %s -", game.gameId.c_str());
    
    logp(identity,0,0,"Sending cards to north player");
    if((ret = game.players[0].sendUserCards(identity)) != 0){
        if(ret == -1){
            //player disconnected need to wai for it
        }else{
            errorp(identity,0,0,"Error sending the cards to north");
        }
    }
    logp(identity,0,0,"Sending cards to east player");
    if((ret = game.players[1].sendUserCards(identity)) != 0){
        if(ret == -1){
            //player disconnected need to wai for it
        }else{
            errorp(identity,0,0,"Error sending the cards to east");
        }
    }
    logp(identity,0,0,"Sending cards to south player");
    if((ret = game.players[2].sendUserCards(identity)) != 0){
        if(ret == -1){
            //player disconnected need to wai for it
        }else{
            errorp(identity,0,0,"Error sending the cards to south");
        }
    }
    logp(identity,0,0,"Sending cards to west player");
    if((ret = game.players[3].sendUserCards(identity)) != 0){
        if(ret == -1){
            //player disconnected need to wai for it
        }else{
            errorp(identity,0,0,"Error sending the cards to west");
        }
    }

    //selecting the dealer
    game.dealer = 0;
    
    bid currentBid;
    int continousPass = 0, i, j, player, goal;
    bool flag = false;

    //inserting dummy bid
    currentBid.val = 0;
    currentBid.trump = 'P';
    game.bids.push_back(currentBid);

    while(continousPass < 3){
        for(i = 0; i < 4; i++ ){
            player = (i + game.dealer)%4;
            flag = false;

            game.players[player].getUserBid(&currentBid, identity);
            
            if(currentBid.val >= 1 && currentBid.val <= 7 ){
                switch(currentBid.trump){
                    case 'd':
                        if( game.declarer != ( (player + 2) % 4) && !game.dbl && !game.redbl){
                            game.dbl = true;
                            continousPass = 0;
                        }else{
                            flag = true;
                        }
                        break;
                    case 'r':
                        if( game.declarer == ( (player + 2) % 4) && !game.dbl && !game.redbl){
                            game.redbl = true;
                            game.dbl = false;
                            continousPass = 0;
                        }else{
                            flag = true;
                        }
                        break;
                    case 'p':
                        continousPass++;
                        break;
                    case 'N': 
                    case 'C': 
                    case 'D': 
                    case 'H': 
                    case 'S':{
                        int lastVal = game.bids.back().val;
                        char lastTrump = game.bids.back().trump;
                        if(currentBid.val > lastVal || (currentBid.val == lastVal  && (currentBid.trump > lastTrump  || currentBid.trump == 'N')) ){
                            game.trump = currentBid.trump;
                            goal = 6 + currentBid.val;
                            game.dbl = false;
                            game.redbl = false;
                            continousPass = 0;
                            if(currentBid.trump != lastTrump  && game.declarer != ((player + 2) % 4) ){
                                game.declarer = player;
                            }
                        }else{
                            flag = true;
                        }
                        break;
                    }
                    default:
                        flag = true;
                }
                if(!flag){
                    game.bids.push_back(currentBid);

                    for(j = 0; j < 4; j++){
                        if(j != player){

                            game.players[j].sendOtherBid(&currentBid, identity);
                        }
                    }
                }
                if(continousPass == 3){
                    break;
                }
            }
        }
    }

    game.setTeam(game.subTeamId, game.players[0].tid, 0, 2, 0);
    game.setTeam(game.subTeamId, game.players[1].tid, 1, 3, 1);

    game.team[game.declarer % 2].setGoal(goal);
    game.team[(game.declarer + 1) % 2].setGoal(13 - goal);

    Card currentCard;
    char currentSuit;
    int winr = game.declarer, k;

    for(i = 0; i < 13; i++){
        Trick trick(winr);

        for(j = 0; j < 4; j++){
            flag = false;
            player = (j + game.declarer) % 4;
            game.players[player].getUserCard(&currentCard, identity);

            if(game.players[player].hasCard(&currentCard, identity)){
                if( j == 0){
                    currentSuit = currentCard.getSuit();
                }else{
                    if(currentCard.getSuit() != currentSuit  && game.players[player].hasCardWithSuit(currentSuit, identity)){
                        //wrong move by player
                        flag = true;
                    }
                }
                if(!flag){
                    trick.addCard(&currentCard);
                    game.players[player].removeCard(&currentCard, identity);
                    for(k = 0; k < 4; k++){
                        if(k != player){
                            game.players[k].sendOtherCard(&currentCard, player, identity);
                        }
                    }
                }
            }else{
                //wrong card by player
            }
        }

        //one trick done
        game.setNextTrick(&trick, identity);
        winr = game.getLastTrickWinner();
        for(k = 0; k < 4; k++){
            if(k != player){
                game.players[k].sendScore(game.team[0].getScore(true),game.team[1].getScore(true), identity);
            }
        }
    }

    //now check for bonuses
//    game.setBonus();
}

void shuffleThread(void* arg){
    playerMsg playerInfo;
    playerInfo = *( (playerMsg*) (arg) );

    string gameId, plid, teamId, name;
    long mtype;
    int fd, playerRecvd = 1, ret;
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

    // Card bd;
    // bd.setRank('A');
    // bd.setSuit('S');
    // bd.setOpen(false); 

    gameA.players[0].sendScore(12,12, identity);
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
                    if(gameA.players[0].tid == teamId){
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
                    if(gameB.players[0].tid == teamId){
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

    Deck deck;
    deck.shuffle();

    for(int i = 0; i < 52 ; i += 4){
        gameA.players[i%4].addCard(deck.deal());
    }

    for(int i = 0; i < 4; i++){
        gameB.players[i].addCard(gameA.players[i].cards);
    }

    shuffleToGameThread gameInfoA, gameInfoB;
    gameInfoA.game = &gameA;
    gameInfoB.game = &gameB;
    
    pthread_t gameAId, gameBId;

    mutexLock(&lock_mapThread, identity, "mapThread");

    logp(identity,0,0,"Calling Calling pthread_create for gameA");
    if((ret = pthread_create(&gameAId, NULL, gameThread, &gameInfoA))!=0) {
        errorp(identity,0,0,"Unable to create the thread");
        debugp(identity,1,ret,"");
    }

    logp(identity,0,0,"Calling Calling pthread_create for gameB");
    if((ret = pthread_create(&gameBId, NULL, gameThread, &gameInfoB))!=0) {
        errorp(identity,0,0,"Unable to create the thread");
        debugp(identity,1,ret,"");
    }

    logp(identity,0,0,"Calling pthread_detach for gameA");
    if ((ret = pthread_detach(gameAId)) != 0) {
        errorp(identity,0,0,"Unable to detach the thread");
        debugp(identity,1,ret,"");
    }

    logp(identity,0,0,"Calling pthread_detach for gameB");
    if ((ret = pthread_detach(gameBId)) != 0) {
        errorp(identity,0,0,"Unable to detach the thread");
        debugp(identity,1,ret,"");
    }
    
    mapThread[gameId+"A"] = gameAId;
    mapThread[gameId+"B"] = gameBId;
    mapThread.erase(gameId);
    mutexUnlock(&lock_mapThread, identity, "mapThread");

    delSetMtype(gameId, identity);

    sleep(2);
    return;
}

void* checkThread(void* arg){
    gameThreadArg playerInfo;
    playerInfo = *( (gameThreadArg*) (arg) );

    int ret, fd;

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

        setMtype(gameId, identity);

        sleep(1);
    }
    else if(mapThread.count(gameId+subTeamId)){//running game
        msg.mtype = mapMtype[gameId+subTeamId];

        sprintf(buf,"Sending msg to queue: mtype(%ld) plid(%s) fd(%d) gameId(%s) - %s", msg.mtype, msg.plid.c_str(), msg.fd, msg.gameId.c_str());
        logp(identity,0,0,buf);
        if(msgSend(&msg, identity) != 0){
            errorp(identity,0,0,"Unable to send the msg");
        }
        logp(identity,0,0,"Message sent successfuly, exiting the thread");
    }else{
        msg.mtype = mapMtype[gameId];

        sprintf(buf,"Sending msg to queue: mtype(%ld) plid(%s) fd(%d) gameId(%s) - %s", msg.mtype, msg.plid.c_str(), msg.fd, msg.gameId.c_str());
        logp(identity,0,0,buf);
        if(msgSend(&msg, identity) != 0){
            errorp(identity,0,0,"Unable to send the msg");
        }
        logp(identity,0,0,"Message sent successfuly, exiting the thread");

    }
}

void setMtype(string gameId, char* identity){
    char cmpltIdentity[CMPLT_IDENTITY_SIZE];
    strcpy(cmpltIdentity, identity);
    strcat(cmpltIdentity,"setMtype");

    int val = 1;
    mutexLock(&lock_VecMtype, cmpltIdentity, "VecMtype");
    vector<int>::iterator it;
    for(it = VecMtype.begin(); it != VecMtype.end(); ++it) {
        if(*it == val){

            val++;
        }
        else{
            break;
        }
    }
    VecMtype.insert(it,val);
    mutexUnlock(&lock_VecMtype, cmpltIdentity, "VecMtype");

    mutexLock(&lock_mapMtype, cmpltIdentity, "mapMtype");
    mapMtype[gameId] =val;
    mutexLock(&lock_mapMtype, cmpltIdentity, "mapMtype");

    return;
}

void delSetMtype(string gameId, char* identity){
    char cmpltIdentity[CMPLT_IDENTITY_SIZE];
    strcpy(cmpltIdentity, identity);
    strcat(cmpltIdentity,"delSetMtype");

    int val = 1;
    mutexLock(&lock_mapMtype, cmpltIdentity, "mapMtype");
    mutexLock(&lock_VecMtype, cmpltIdentity, "VecMtype");
    vector<int>::iterator it;
    for(it = VecMtype.begin(); it != VecMtype.end(); ++it) {
        if(*it == val){

            val++;
        }
        else{
            break;
        }
    }
    VecMtype.insert(it,val);
    mutexUnlock(&lock_VecMtype, cmpltIdentity, "VecMtype");

    mapMtype[gameId+'A'] =val;

    val = 1;
    mutexLock(&lock_VecMtype, cmpltIdentity, "VecMtype");
    
    for(it = VecMtype.begin(); it != VecMtype.end(); ++it) {
        if(*it == val){

            val++;
        }
        else{
            break;
        }
    }
    VecMtype.insert(it,val);
    mutexUnlock(&lock_VecMtype, cmpltIdentity, "VecMtype");

    mapMtype[gameId+'B'] =val;

    val = mapMtype[gameId];
    for(vector<int>::iterator it = VecMtype.begin(); it != VecMtype.end(); ++it) {
        if(*it == val){

            break;
        }
    }
    VecMtype.erase(it);
    mutexUnlock(&lock_VecMtype, cmpltIdentity, "VecMtype");

    mapMtype.erase(gameId);
    mutexLock(&lock_mapMtype, cmpltIdentity, "mapMtype");

    return;
}


//class Card
Card::Card(char rank, char suit, bool open = false)
    :rank(rank), suit(suit), open(open)
{}

Card::Card(){
    open = false;
}

string Card::print(){
    std::ostringstream oss;
    oss << "Card's Rank: '" << rank << "' and Suit: '"<< suit <<"'";
    return oss.str();
}

string Card::format_json(){
    std::ostringstream oss;
    char openChar = (open ? 'T' : 'F');

    oss << "{\"rank\":\"" << rank << "\", \"suit\": \""<< suit <<"\", \"open\":\"" << openChar << "\"}";
    return oss.str();    
}
char Card::getRank(){
    return rank;
}
char Card::getSuit(){
    return suit;
}
void Card::setRank(char r){
    rank = r;
}
void Card::setSuit(char s){
    suit = s;
}
bool Card::getOpen(){
    return open;
}
void Card::setOpen(bool b){
    open = b;
}


//class Deck
Deck::Deck(){
    int i,j;

    for(i = 0; i < 4; i++){
        for(j = 0; j < 13; j++){
            Card c(RANKS[j],SUITS[i], false);
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
Trick::Trick(int first) //N,W,E,S = 0,1,2,3
    :first(first)
{
    i = 0;
}

Trick::Trick(){
    i = 0;
}

string Trick::print(){
    std::ostringstream oss;
    oss << "Card1 played by '"<< first << "': " << cards[0].print() << ", Card2 played by '"<< nextPos(first) << "': " << cards[1].print() << ", Card3 played by '"<< nextPos(first) << "': " << cards[2].print() << ", Card4 played by '"<< nextPos(first) << "': " << cards[3].print() ; 
    return oss.str();
}

void Trick::addCard(Card* c){
    cards[i++] = *c;
}

Card Trick::getCard(int i){
    return cards[i];
}

void Trick::setWinTeam(char trump){
    int i;
    char winRank = cards[0].getRank(), winSuit = cards[0].getSuit();
    winTeam = 0;
    winner = 0;
    for(i = 1; i < 4; i++){
        if(cards[i].getSuit() == winSuit){
            if(cards[i].getRank() > winRank){
                winRank = cards[i].getRank();
                winSuit = cards[i].getSuit();
                winTeam = i % 2;
                winner = i;
            }
        }else if(cards[i].getSuit() == trump){
            winRank = cards[i].getRank();
            winSuit = cards[i].getSuit();
            winTeam = i % 2;
            winner = i;
        }
    }
}

void Trick::setScore(char trump, bool dbl, bool redbl, bool initial, bool belowTheLine, bool vulnerable){
    int factor = 1;

    if(belowTheLine || (!belowTheLine && !dbl && !redbl)){
        factor = dbl ? 2 : 1 ;
        factor = redbl ? 4 : 1 ;

        if(trump = 'N'){
            if(initial){
                score = factor * 40;
            }else{
                score = factor * 30;
            }
        }else if(trump == 'C' || trump == 'D'){
            score = factor * 20;
        }else {
            score = factor * 30;
        }
    }else{
        factor = vulnerable ? 2 : 1 ;
        score = dbl ? factor * 100 : factor * 200;
    }
}

void Trick::setScore(int sc){
    score = sc;
}

int Trick::getWinTeam(){
    return winTeam;
}

int Trick::getWinner(){
    return winner;
}

int Trick::getScore(){
    return score;
}

//class Player
Player::Player(string plid, char position, char team, string tid, string name, int fd)
    :plid(plid), position(position), subTeamId(team), tid(tid), name(name), fd(fd)
{}

void Player::addCard(Card c){
    cards.push_back(c);
}

void Player::addCard(vector <Card> &crds){
    cards = crds;
}

Player::Player()
{}
// int Player::recvUser(){

// }

int Player::sendOtherCard(Card* c, int pos, char* identity){
    char cmpltIdentity[CMPLT_IDENTITY_SIZE], buf[150];
    strcpy(cmpltIdentity, identity);
    strcat(cmpltIdentity,"-Player::sendOtherCard");

    logp(cmpltIdentity,0,0,"Making the data to send");
    std::ostringstream oss;
    oss << "CARDO{\"pos\":\"" << pos << "\", \"card\":" << c->format_json() << "}";
     
    logp(cmpltIdentity,0,0,"Converting the data to string");
    string s = oss.str();
    int ret, len = s.length();

    sprintf(buf, "Sending card to client ,totalLength(%d), command(%s)",len, s.substr(0,5).c_str());//length is 47
    logp(identity,0,0,buf);
    if((ret = sendall(fd, s.c_str(), &len, 0) ) != 0){
        errorp(identity, 0, 0, "Unable to send complete data");
        debugp(identity,1,errno,"");
    }
    sprintf(buf, "Returning with return value(%d)",ret);
    logp(cmpltIdentity,0,0,buf);
    return ret;
}

int Player::sendUserCards(char* identity){
    char cmpltIdentity[CMPLT_IDENTITY_SIZE], buf[150];
    strcpy(cmpltIdentity, identity);
    strcat(cmpltIdentity,"-Player::sendUserCards");

    logp(cmpltIdentity,0,0,"Making the data to send");
    std::ostringstream oss;
    oss << "CARDS{\"cards\":[";
    
    vector<Card>::iterator end = --(cards.end());
    for(vector<Card>::iterator it = cards.begin(); it != end; ++it) {
        oss << (*it).format_json() << ", ";
    }
    oss << (*it).format_json() << " ] }";
    
    logp(cmpltIdentity,0,0,"Converting the data to string");
    string s = oss.str();
    int ret, len = s.length();

    sprintf(buf, "Sending cards to client ,totalLength(%d), command(%s)",len, s.substr(0,5).c_str());//length is 368
    logp(identity,0,0,buf);
    if((ret = sendall(fd, s.c_str(), &len, 0) ) != 0){
        errorp(identity, 0, 0, "Unable to send complete data");
        debugp(identity,1,errno,"");
    }
    sprintf(buf, "Returning with return value(%d)",ret);
    logp(cmpltIdentity,0,0,buf);
    return ret;
}

int Player::sendOtherBid(bid* bd, char* identity){
    char cmpltIdentity[CMPLT_IDENTITY_SIZE], buf[150];
    strcpy(cmpltIdentity, identity);
    strcat(cmpltIdentity,"-Player::sendOtherBid");

    logp(cmpltIdentity,0,0,"Making the data to send");
    std::ostringstream oss;
    oss << "BIDOT{\"bid\":{\"val\":\"" << bd->val << "\", \"trump\":\"" << bd->trump << "\"}}";
     
    logp(cmpltIdentity,0,0,"Converting the data to string");
    string s = oss.str();
    int ret, len = s.length();

    sprintf(buf, "Sending bid to clients ,totalLength(%d), command(%s)",len, s.substr(0,5).c_str());//length is 368
    logp(identity,0,0,buf);
    if((ret = sendall(fd, s.c_str(), &len, 0) ) != 0){
        errorp(identity, 0, 0, "Unable to send complete data");
        debugp(identity,1,errno,"");
    }
    sprintf(buf, "Returning with return value(%d)",ret);
    logp(cmpltIdentity,0,0,buf);
    return ret;
}

int Player::getUserBid(bid* bd, char* identity){
    char cmpltIdentity[CMPLT_IDENTITY_SIZE], buf[150];
    strcpy(cmpltIdentity, identity);
    strcat(cmpltIdentity,"-Player::getUserBid");

    int ret, len = 5;
    char command[5];

    logp(cmpltIdentity,0,0,"Receiving the command");
    if((ret = recvall(fd, command, &len, 0)) != 0) {
        errorp(identity,0,0,"Unable to recv the command");
        debugp(identity,1,errno,"");
        return -5;
    }

    if(strncmp(command, "BIDMY", 5) == 0){
        len = commandsDataLen["BIDMY"];
        char dataC[len];
        logp(cmpltIdentity,0,0,"Receiving the data");
        if((ret = recvall(fd, dataC, &len, 0)) != 0) {
            errorp(identity,0,0,"Unable to recv the data");
            debugp(identity,1,errno,"");
            return -3;
        }
        
        string data;
        data.assign(dataC, len);

        Json::Value root;
        Json::Reader reader;        
        
        logp(cmpltIdentity,0,0,"Parsing data");
        if( !reader.parse(data, root, false) ){
            errorp(identity,0,0,"Error parsing the data");
            debugp(identity,0,0,reader.getFormatedErrorMessages().c_str());
            return -2;
        }

        bd->val = root["bid"]["val"].asInt();
        bd->trump = root["bid"]["trump"].asString().at(0);

        return 0;
    }else{//wrong command received
        errorp(identity,0,0,"Wrong command recvd");
        return -4;
    }

}

int Player::getUserCard(Card* c, char* identity){
    char cmpltIdentity[CMPLT_IDENTITY_SIZE], buf[150];
    strcpy(cmpltIdentity, identity);
    strcat(cmpltIdentity,"-Player::getUserCard");

    int ret, len = 5;
    char command[5];

    logp(cmpltIdentity,0,0,"Receiving the command");
    if((ret = recvall(fd, command, &len, 0)) != 0) {
        errorp(identity,0,0,"Unable to recv the command");
        debugp(identity,1,errno,"");
        return -5;
    }

    if(strncmp(command, "CARDM", 5) == 0){
        len = commandsDataLen["CARDM"];
        char dataC[len];
        logp(cmpltIdentity,0,0,"Receiving the data");
        if((ret = recvall(fd, dataC, &len, 0)) != 0) {
            errorp(identity,0,0,"Unable to recv the data");
            debugp(identity,1,errno,"");
            return -3;
        }
        
        string data;
        data.assign(dataC, len);

        Json::Value root;
        Json::Reader reader;        
        
        logp(cmpltIdentity,0,0,"Parsing data");
        if( !reader.parse(data, root, false) ){
            errorp(identity,0,0,"Error parsing the data");
            debugp(identity,0,0,reader.getFormatedErrorMessages().c_str());
            return -2;
        }

        c->setRank( root["card"]["rank"].asString().at(0) );
        c->setSuit( root["card"]["suit"].asString().at(0) );
        c->setOpen( (root["card"]["open"].asString().at(0) == 'T' ? true : false) );

        return 0;
    }else{//wrong command received
        errorp(identity,0,0,"Wrong command recvd");
        return -4;
    }

}

bool Player::hasCard(Card* c, char* identity){
    char cmpltIdentity[CMPLT_IDENTITY_SIZE], buf[150];
    strcpy(cmpltIdentity, identity);
    strcat(cmpltIdentity,"-Player::hasCard");

    logp(cmpltIdentity,0,0,"Starting the iteration");
    vector<Card>::iterator it;
    for(it = cards.begin(); it != cards.end(); it++){
        if((*it).getSuit() == c->getSuit()  && (*it).getRank() == c->getRank() && !(*it).getOpen()){
            logp(cmpltIdentity,0,0,"Returning True");
            return true;
        }
    }

    logp(cmpltIdentity,0,0,"Returning False");
    return false;
}

bool Player::hasCardWithSuit(char s, char* identity){
    char cmpltIdentity[CMPLT_IDENTITY_SIZE], buf[150];
    strcpy(cmpltIdentity, identity);
    strcat(cmpltIdentity,"-Player::hasCardWithSuit");

    logp(cmpltIdentity,0,0,"Starting the iteration");
    vector<Card>::iterator it;
    for(it = cards.begin(); it != cards.end(); it++){
        if((*it).getSuit() == s && !(*it).getOpen()){
            logp(cmpltIdentity,0,0,"Returning False");
            return false;
        }
    }

    logp(cmpltIdentity,0,0,"Returning True");
    return true;
}

void Player::removeCard(Card* c, char* identity){
    char cmpltIdentity[CMPLT_IDENTITY_SIZE], buf[150];
    strcpy(cmpltIdentity, identity);
    strcat(cmpltIdentity,"-Player::removeCard");

    logp(cmpltIdentity,0,0,"Starting the iteration");
    vector<Card>::iterator it;
    for(it = cards.begin(); it != cards.end(); it++){
        if((*it).getSuit() == c->getSuit()  && (*it).getRank() == c->getRank() ){
            logp(cmpltIdentity,0,0,"Removing i.e. opening the Card");
            //cards.erase(it);
            (*it).setOpen(true);
            return;
        }
    }

    errorp(cmpltIdentity,0,0,"No card found.This should not be happening");
    return;
}

bool Player::fourTrumpHonor(char trump, char* identity){
    char cmpltIdentity[CMPLT_IDENTITY_SIZE], buf[150];
    strcpy(cmpltIdentity, identity);
    strcat(cmpltIdentity,"-Player::fourTrumpHonor");

    int cnt = 0;

    logp(cmpltIdentity,0,0,"Starting the iteration");
    vector<Card>::iterator it;
    for(it = cards.begin(); it != cards.end(); it++){
        if((*it).getSuit() == trump  && isHonor((*it).getRank()) ){
            logp(cmpltIdentity,0,0,"found a honor Card");
            cnt++;
        }
    }

    sprintf(buf, "returning with cnt(%d), if four  = true", cnt);
    logp(cmpltIdentity,0,0, buf);

    if(cnt == 4){
        return true;
    }else{
        return false;
    }
}

bool Player::fiveTrumpHonor(char trump, char* identity){
    char cmpltIdentity[CMPLT_IDENTITY_SIZE], buf[150];
    strcpy(cmpltIdentity, identity);
    strcat(cmpltIdentity,"-Player::fiveTrumpHonor");

    int cnt = 0;

    logp(cmpltIdentity,0,0,"Starting the iteration");
    vector<Card>::iterator it;
    for(it = cards.begin(); it != cards.end(); it++){
        if((*it).getSuit() == trump  && isHonor((*it).getRank()) ){
            logp(cmpltIdentity,0,0,"found a honor Card");
            cnt++;
        }
    }

    sprintf(buf, "returning with cnt(%d), if five  = true", cnt);
    logp(cmpltIdentity,0,0, buf);
    
    if(cnt == 5){
        return true;
    }else{
        return false;
    }
}

bool Player::fourAces(char trump, char* identity){
    char cmpltIdentity[CMPLT_IDENTITY_SIZE], buf[150];
    strcpy(cmpltIdentity, identity);
    strcat(cmpltIdentity,"-Player::fourAces");

    if(trump == 'N'){
        int cnt = 0;

        logp(cmpltIdentity,0,0,"Starting the iteration");
        vector<Card>::iterator it;
        for(it = cards.begin(); it != cards.end(); it++){
            if((*it).getRank() == 'A' ){
                logp(cmpltIdentity,0,0,"found a Ace");
                cnt++;
            }
        }

        sprintf(buf, "returning with cnt(%d), if four  = true", cnt);
        logp(cmpltIdentity,0,0, buf);
        
        if(cnt == 4){
            return true;
        }else{
            return false;
        }
    }else{
        logp(cmpltIdentity,0,0, "Trump is not no Trump, so returning false");
        return false;
    }
}

int Player::sendScore(int tm1Score, int tm2Score, char* identity){
    char cmpltIdentity[CMPLT_IDENTITY_SIZE], buf[150];
    strcpy(cmpltIdentity, identity);
    strcat(cmpltIdentity,"-Player::sendScore");

    string s;

    logp(cmpltIdentity,0,0,"Making the data to send and Converting the data to string");
    std::ostringstream oss1, oss2;
    
    oss1 << "{\"team1Score\":\"" << tm1Score << "\", \"team2Score\":\"" << tm2Score << "\"}";
    s = oss1.str();
    oss2 << "SCORE" << s.length();
    s = oss2.str() + s; 
    
    int ret, len = s.length();
    cout << s <<endl;
    sprintf(buf, "Sending card to client ,totalLength(%d), command(%s)",len, s.substr(0,5).c_str());//length is 47
    logp(identity,0,0,buf);
    if((ret = sendall(fd, s.c_str(), &len, 0) ) != 0){
        errorp(identity, 0, 0, "Unable to send complete data");
        debugp(identity,1,errno,"");
    }
    sprintf(buf, "Returning with return value(%d)",ret);
    logp(cmpltIdentity,0,0,buf);
    return ret;
}

//class Team
Team::Team(char team, string tid,int p1, int p2)
    :subTeamId(team), teamId(tid), pl1Pos(p1), pl2Pos(p2)
{
    scoreBTL = 0;
    scoreATL = 0;
    goal = 6;
    done = 0;//starting from [1 , 13]
    vulnerable = false;
}

Team::Team(){
    scoreBTL = 0;
    scoreATL = 0;
    goal = 6;
    done = 0;
    vulnerable = false;
}

void Team::setFields(char stm, string tid,int p1, int p2){
    subTeamId = stm;
    teamId = tid;
    pl1Pos = p1;
    pl2Pos = p2;
}

void Team::setGoal(int g){
    goal = g;
}

int Team::getGoal(){
    return goal;
}

void Team::setDone(int d){
    done = d;
}

int Team::getDone(){
    return done;
}

void Team::increaseScore(int s, bool btl){
    if(btl){
        scoreBTL += s;
    }else{
        scoreATL += s;
    }
}

int Team::getScore(bool btl){
    if(btl){
        return scoreBTL;
    }else{
        return scoreATL;
    }
}
//class game
Game::Game(string gid, char stid)
    :gameId(gid), subTeamId(stid)
{
    dbl = false;
    redbl = false;
    index = 0;
}

Game::Game(string gid, char stid, Player n,Player s, Player e, Player w)
    :gameId(gid), subTeamId(stid)
{
    dbl = false;
    redbl = false;
    index = 0;
    players[0] = n;
    players[1] = s;
    players[2] = e;
    players[3] = s;
}


void Game::setPlayer(Player& p, char pos){
    switch(pos) {
        case 'N':
            players[0] = p;
            break;
        case 'E':
            players[1] = p;
            break;
        case 'S':
            players[2] = p;
            break;
        case 'W':
            players[3] = p;
            break;
    }    
}

void Game::setTeam(char stm, string tid,int p1, int p2, int which){
    team[which].setFields(stm, tid, p1, p2);
}

void Game::addTrick(Trick* t){
    tricks[index++] = *t;
}

Trick Game::getTrick(int i){
    return tricks[i];
}

void Game::setNextTrick(Trick* trick, char* identity){
    int winner, winTeam;
    bool initial;
    trick->setWinTeam(trump);
    winTeam = trick->getWinTeam();
    winner = trick->getWinner();

    if(team[winTeam].getDone() < 6){
        trick->setScore(0);
    }else if(team[winTeam].getDone() == 6){
        initial = true;
    }else{
        initial = false;
    }
    team[winTeam].setDone(team[winTeam].getDone() + 1);

    bool belowTheLine = ( team[winTeam].getGoal() >= team[winTeam].getDone() );
    trick->setScore(trump, dbl, redbl, initial, belowTheLine , team[winTeam].vulnerable);

    tricks[index++] = *trick;

    team[winTeam].increaseScore(trick->getScore(), belowTheLine);
}

int Game::getLastTrickWinner(){
    return tricks[index - 1].getWinner();
}

void Game::setBonusPenalties(char *identity){
    char cmpltIdentity[CMPLT_IDENTITY_SIZE], buf[150];
    strcpy(cmpltIdentity, identity);
    strcat(cmpltIdentity,"-Game::setBonusPenalties");

    int overtricks = team[declarer % 2].getDone() - team[declarer % 2].getGoal();
    int scoreATL, scoreBTL;
    bool vulnerable = team[declarer % 2].vulnerable;

    scoreBTL = team[declarer % 2].getScore(true);
    scoreATL = team[declarer % 2].getScore(false);

    if(overtricks >= 0){
        if(scoreBTL >= 100){
            scoreATL += vulnerable ? 500 : 300;
        }

        if(team[declarer % 2].getGoal() == 12){
            scoreATL += vulnerable ? 750 : 500;
        }else if(team[declarer % 2].getGoal() == 13){
            scoreATL += vulnerable ? 1500 : 1000;
        }

        if(dbl){
            scoreATL += 50;
        }else if(redbl){
            scoreATL += 100;
        }

        if(game.players[declarer].fourTrumpHonor(trump, cmpltIdentity) || game.players[(declarer + 2) % 4].fourTrumpHonor(trump, cmpltIdentity) ){
            scoreATL += 100;
        }else if(game.players[declarer].fiveTrumpHonor(trump, cmpltIdentity) || game.players[(declarer + 2) % 4].fiveTrumpHonor(trump, cmpltIdentity) ){
            scoreATL += 150;
        }

        if(game.players[declarer].fourAces(trump, cmpltIdentity) || game.players[(declarer + 2) % 4].fourAces(trump, cmpltIdentity) ){
            scoreATL += 150;
        }    
    }else { //penalties

    }
}

bool isHonor(char r){
    switch(r){
        case 'T':
        case 'J':
        case 'Q':
        case 'K':
        case 'A':
            return true;
            break;
        default:
            return false;
    }
}