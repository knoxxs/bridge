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
char nextPos(char);

class Card{
	public:
		Card(char, char, bool);
		Card();
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
		Trick();
		string print();
		Card nextCard(int);
		void nextCard(Card);
		void setWinner();
		char getWinner();
		int score();
	private:
		Card cards[4];
		char first;
		int i;
		char winner;
};

class Tricks{
	public:
		Tricks();
		string print();
		void add(Trick);
		Trick get(int);
		void setWinner();
		char getWinner();
		int score(char);
	private:
		Trick tricks[13];
		int index;
		char winner;
};
class Player{
	public:
		char position;
		string plid;
		char subTeamId;
		string tid, name, country;
		int fd;
		vector <Card> cards;
		Player(string, char, char, string, string, string, int);
		int getUserChoice();
	private:
};
class Team{
	public:
		Team(char, string,string, string);
		int score();
	private:
		char team;
		string tid, plid1, plid2;
};
class Game{
	public:
		Game(string, char, Player& ,Player& , Player&, Player&);
		Game(string, char);
		int score();
		void setPlayer(Player&, char);
		Player &N,&S,&E,&W;
		string gameId;
		char subTeamId;
		char declarer, trump;
		int goal;
	private:
};

#endif