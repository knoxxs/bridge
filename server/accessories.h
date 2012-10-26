#include <malloc.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <error.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <stropts.h>

#define PORT "4444" //port we are listening on

#define PASSCHECK "password"


int sendall(int fd, char *buf, int *len);
int recvall(int fd, char *buf, int *len);
void logp(int typ, char* msg);
void errorp(char *where, int boolean, int errn,char *what);