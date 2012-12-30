#ifndef helper
#define helper

#include <string>
 

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
#define LOG_PATH "./log"
#define CMPLT_IDENTITY_SIZE 100

using namespace std;

int unixSocket(char* ,char *, int);
int recv_fd(int , ssize_t (*userfunc)(int, const void *, size_t), char*, int len, char*);
ssize_t errcheckfunc(int, const void *, size_t, char*);
int getPlayerInfo(char *, char *, char *, int, int, int, int, char*);
int send_fd(int, int, char*, int len, char*);
int unixClientSocket(char* ,char*, int len);
int send_err(int, int, const char *, int len);
void mutexUnlock(pthread_mutex_t *, char*, string);
void mutexLock(pthread_mutex_t *, char*, string);


#endif