#include <iostream>
#include <libpq-fe.h>
#include <string.h>
#include <string>
using namespace std;
//sudo g++ PSQLTest.cpp -I /usr/include/postgresql -lpq -o PSQLTest
//sudo -u postgres psql #
//global variable*************************************************************8
string** query_temp;
//********************************************************************
//dynamic array*************************************************************

string** AllocateDynamicArray( int nRows, int nCols)
{
      string **dynamicArray;

      dynamicArray = new string*[nRows];
      for( int i = 0 ; i < nRows ; i++ )
      dynamicArray[i] = new string[nCols];

      return dynamicArray;
}

void FreeDynamicArray(string** dArray)
{
      delete [] *dArray;
      delete [] dArray;
}
//********************************************************************************

//database functions-*************************************************

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

void gen_query(PGconn *conn,string s)
{
    char * c;
        c = new char[s.length() + 1];
        strcpy(c, s.c_str());
    PGresult *res = PQexec(conn,c);

if (PQresultStatus(res) == PGRES_TUPLES_OK)
{
  cout << "query executed successfully\n";

    
    int row_num = PQntuples(res); // number of rows in the output of the query
    int col_num = PQnfields(res); // number of columns in the output of the query
    query_temp = AllocateDynamicArray(row_num,col_num);
    for(int i=0;i<row_num;i++)
    {
        for(int j=0;j<col_num;j++)
        {
            query_temp[i][j] = PQgetvalue(res,i,j);
            cout<<query_temp[i][j]<<endl;
            //cout<<a[i][j]<<endl;
        }
    }
    
    //FreeDynamicArray(abc);

  // Clear result
  //PQclear(res);
}      
      //successful completion of a command returning NO data
  if ((PQresultStatus(res) != PGRES_COMMAND_OK) && (PQresultStatus(res) != PGRES_TUPLES_OK) )
  {
    cout << "Query failed\n";
    PQclear(res);
    CloseConn(conn);
  }
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


//**********************************************************************

//************************************************************************
void check_login(string recv)
{
    string id = recv.substr(0,8);
    cout<<id<<endl;
    int length = recv.length();
    string pass = recv.substr(9,length);
    cout<<pass<<endl;
    
    cout<<id.compare(query_temp[0][0])<<endl;
    cout<<pass.compare(query_temp[0][1])<<endl;
    if((id.compare(query_temp[0][0])==0) && (pass.compare(query_temp[0][1])==0))
    {
        cout<<"valid user";
    }
    else
    {
        cout<<"invalid";
    }
    FreeDynamicArray(query_temp);
}
//**************************************************************************
int main()
{
  PGconn *conn = NULL;
  //conn = ConnectDB("postgres","123321","bridge","127.0.0.1","5432");
  conn = ConnectDB("postgres","123321","bridge","127.0.0.1","5432");
  
  if (conn != NULL) {
    login(conn, "11111000", "abcd");
    CloseConn(conn);
  }

  return 0;

}