#ifndef msgQ
#define msgQ

#include <string>

#define MSGQFLAG IPC_CREAT|0666
#define MSGQKEY 4444
#define CMPLT_IDENTITY_SIZE 100
#define MSGSENDFLAG IPC_NOWAIT
#define MSGRECVFLAG 0

using namespace std;

struct playerMsg{
    long mtype;
    string plid;
    int fd;
    string gameId;
};

void createMsgQ(char *);
int msgSend(playerMsg* , char *);
int msgRecv(playerMsg* , int , char *);


#endif