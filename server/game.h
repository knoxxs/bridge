#ifndef GAME_H
#define GAME_H

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
void setMtype(string, char*);
void delSetMtype(string, char*);

struct bid{
	int val;// A, 2 ... K  //pass,double,redouble = 0
	char trump;//C D H S N    //p     d     r
};

class Card{
	public:
		Card(char, char, bool);
		Card();
		string print();
		char getSuit();
		char getRank();
		void setRank(char);
		void setSuit(char);
		bool getOpen();
		void setOpen(bool);
		string format_json();
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
		Trick(int);
		Trick();
		string print();
		Card getCard(int);
		void addCard(Card*);
		void setWinner();
		char getWinner();
		int score();
	private:
		Card cards[4];
		int first, i, winner;
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
		Player(string, char, char, string, string, int);
		Player();
		void addCard(vector <Card> &);
		void addCard(Card);
		int sendUserCards(char*);
		int sendOtherCard(Card* , int, char*);
		int getUserBid(bid*, char*);
		int sendOtherBid(bid*, char*);
		int getUserCard(Card*, char*);
		bool hasCard(Card*, char*);
		bool hasCardWithSuit(char, char*);
		void removeCard(Card*, char*);
	private:
};
class Game{
	public:
		Game(string, char, Player, Player, Player, Player);
		Game(string, char);
		void setPlayer(Player&, char);
		int score();
		Player players[4];
		string gameId;
		char subTeamId, trump;
		int declarer, dealer;
		bool dbl, redbl;
		vector<bid> bids;
		int goal;
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

#endif