#ifndef access
#define access

#include <string>

#define PORT "4444" //port we are listening on
#define PASSCHECK "password"
#define LOG_TYPE   1 
#define WARN_TYPE  2 
#define ERROR_TYPE 3 
#define DEBUG_TYPE 4

using namespace std;


void setLogFile(int);
int sendall(int fd, char *buf, int *len, int flags);
int recvall(int fd, char *buf, int *len, int flags);
void logWrite(int typ, string msg);
void errorp(string where, int boolean, int errn,string what);
void logp(string where, int boolean, int errn,string what);
void debugp(string where, int boolean, int errn,string what);

class Time{
private:
	int year;
	int month;
	int date;
	int hour;
	int min;
	int sec;
public:
Time(string s);
int getYear();
int getMonth();
int getDate();
int getHour();
int getMin();
int getSec();
};

#endif