#ifndef player_helperFlag
#define player_helperFlag
#include <unistd.h>

#define PLAYER_NAME_SIZE 31 //30 + 1
#define PLAYER_TEAM_SIZE 9 //8+1


int unixSocket();
int recv_fd(int , ssize_t (*userfunc)(int, const void *, size_t), char*);
ssize_t errcheckfunc(int, const void *, size_t);

#endif