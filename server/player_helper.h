#ifndef player_helperFlag
#define player_helperFlag
#include <unistd.h>



int unixSocket();
void connection_handler(int);
int recv_fd(int , ssize_t (*userfunc)(int, const void *, size_t));
ssize_t errcheckfunc(int, const void *, size_t);

#endif