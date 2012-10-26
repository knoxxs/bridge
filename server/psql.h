#include <iostream>
#include <libpq-fe.h>
#include <string>

using namespace std;

PGconn *ConnectDB(string ,string, string, string, string);
void login(PGconn, string, string);
void CloseConn(PGconn);