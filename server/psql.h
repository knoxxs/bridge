#ifndef psql
#define psql

using namespace std;

PGconn *ConnectDB(string ,string, string, string, string);
int login_check(PGconn*, string, string);
void CloseConn(PGconn*);

#endif