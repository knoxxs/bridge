#include "psql_player.h"
#include <iostream>
#include <string.h>

void CloseConn(PGconn *conn)
{
  PQfinish(conn);
  //getchar();
}

PGconn *ConnectDB(string user="postgres",string password="123321",string dbname="bridge",string hostaddr="127.0.0.1",string port="5432")
{
  PGconn *conn = NULL;
  string s = "user=" + user + " password=" + password + " dbname=" +dbname + " hostaddr=" + hostaddr + " port=" + port;

  // Make a connection to the database
  conn = PQconnectdb(s.c_str());

  // Check to see that the backend connection was successfully made
  if (PQstatus(conn) != CONNECTION_OK)
  {
    cout << "Connection to database failed.\n";
    CloseConn(conn);
  }

  cout << "Connection to database - OK\n";

  return conn;
}

int getPlayerInfoFromDb(PGconn *conn, string plid, char* name, char* team){
    cout << plid << endl;
    string query = "SELECT * FROM players where pid='" + plid + "'";

    PGresult *res = PQexec(conn,query.c_str());

    if(PQresultStatus(res) == PGRES_TUPLES_OK) { //successful completion of a command returning data
        cout << "query executed successfully\n";
        int row = PQntuples(res); // number of rows in the output of the query
        if( row == 1){
            //TODO check how mwmory is located for returned value and their scope
            //I think they are declared in this function.
            strcpy( name, PQgetvalue(res,0,2) );
            strcpy( team, PQgetvalue(res,0,1) );
            //printf("bla bla%s %s\n",name, team );
            PQclear(res);
            return 0;
        }
        else {
            printf("error in row else of getplayerInfoFromDb \n");
            //error
            return -1;
        }
    }
    else {
        printf("error in query executuin - else of getplayerInfoFromDb \n");
        //error
        return -2;
    }
}