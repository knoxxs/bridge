#include <libpq-fe.h>
#include <string>
using namespace std;

PGconn *ConnectDB(string ,string, string, string, string);
int login_check(PGconn*, string, string);
void CloseConn(PGconn*);