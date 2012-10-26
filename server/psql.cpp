#include "psql.h"

void CloseConn(PGconn *conn)
{
  PQfinish(conn);
  getchar();
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


void login(PGconn *conn, string username, string password)
{
  string query = "SELECT * FROM login where plid='" + username + "'";

  PGresult *res = PQexec(conn,query.c_str());

  if(PQresultStatus(res) == PGRES_TUPLES_OK)//successful completion of a command returning data
  {
    cout << "query executed successfully\n";
    int row = PQntuples(res); // number of rows in the output of the query
    cout<<row<<endl;
    if (row != 1)
    {
      //wrong username
    }
    else
    {
      //Must be 2 so no need to check
      //int col_num = PQnfields(res); // number of columns in the output of the query
      // for(int i=0;i<row_num;i++)
      // {
      //     for(int j=0;j<col_num;j++)
      //     {
      //         query_temp[i][j] = PQgetvalue(res,i,j);
      //         cout<<query_temp[i][j]<<endl;
      //         //cout<<a[i][j]<<endl;
      //     }
      // }
      //no need to check username //PQgetvalue(res,0,0);
      
      //string ActualPass(cstring);
      cout<<PQgetvalue(res,0,1)<<endl;
      if( !(string(PQgetvalue(res,0,1)).compare(password)) )//return 0 on equality
      {
        cout<<"valid user";
      }
    }
  }

  // Clear result
  PQclear(res);
}