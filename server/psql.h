#ifndef psql
#define psql

#include <libpq-fe.h>
#include <string>

#define CMPLT_IDENTITY_SIZE 80

using namespace std;

int ConnectDB(string ,string, string, string, string, char*);
int login_check(string, string, char*);
void CloseConn(char*);
int getPlayerInfoFromDb(string, char*, char*, char*);
int getPlayerSchedule(string , string& ,char* );

#endif