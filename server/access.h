#ifndef access
#define access

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
#include <regex.h>
#include <math.h>

#define PORT "4444" //port we are listening on
#define PASSCHECK "password"
#define LOG_TYPE   1 
#define WARN_TYPE  2 
#define ERROR_TYPE 3 
#define DEBUG_TYPE 4


int logfile;
void setLogFile(int);
int sendall(int fd, char *buf, int *len);
int recvall(int fd, char *buf, int *len);
void logWrite(int typ, char* msg);
void errorp(char *where, int boolean, int errn,char *what);
void logp(char *where, int boolean, int errn,char *what);
void debugp(char *where, int boolean, int errn,char *what);

#endif