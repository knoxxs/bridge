#ifndef game
#define game

#include <string>

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
		print();
		getSuit();
		getRank();
	private:
		char suit;
		char rank;
};

class Deck{
	public:
		Deck();
		print();
		shuffle();
};


#endif