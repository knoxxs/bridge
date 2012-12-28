#ifndef game
#define game

#include <string>
#include <vector> 
using namespace std;


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

int unixSocket();
int recv_fd(int , ssize_t (*userfunc)(int, const void *, size_t), char*, int len);
ssize_t errcheckfunc(int, const void *, size_t);
void* gameMain(void*);
void connection_handler(int);
int getPlayerInfo(char *, char *, char *, int, int, int, int);

class Card{
	public:
		Card(char, char);
		string print();
		char getSuit();
		char getRank();
	private:
		char suit;
		char rank;
		bool open;
};

class Deck{
	public:
		Deck();
		string print();
		void shuffle();
		Card deal();
	private:
		vector<Card> deck;
};
class Trick{
	public:
		Trick(char);
		string print();
		Card nextCard(int);
		void nextCard(Card);
		int score();
	private:
		Card cards[4];
		char first;
};
class Tricks{
	public:
		Tricks();
		string print();
		void add(Trick);
		Trick get(int);
		char getWinner();
		int score(char);
	private:
		Trick tricks[13];
		char winner[13];
		int index;
};
class Player{
	public:
		Player();
		int getUserChoice();
	private:
		char position;
		string plid;
		char team;
		string tid, name, country;
};
class Team{
	public:
		int score();
	private:
		char team;
		string plid1, plid2;

};

#endif