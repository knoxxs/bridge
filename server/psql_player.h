#include <libpq-fe.h>
#include <string>
using namespace std;

PGconn *ConnectDB(string ,string, string, string, string);
int getPlayerInfoFromDb(PGconn *, string, char*, char*);
void CloseConn(PGconn*);