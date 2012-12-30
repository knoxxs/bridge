#ifndef game
#define game

#include <string>
#include <vector> 
using namespace std;


/* size of control buffer to send/recv one file descriptor */
#define UNIX_SOCKET_FILE_PLA_TO_GAM "./UNIX_SOCKET_FILE_PLA_TO_GAM"
#define LISTEN_QUEUE_SIZE 10

void* checkThread(void*);
void connection_handler(int);
void shuffleThread(void*);
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
		Game(string, char, Player ,Player , Player, Player);
		int score();
		Player N,S,E,W;
		string gameId;
		char subTeamId;
		char declarer, trump;
		int goal;
	private:
};

#endif