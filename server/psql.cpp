#include <libpq-fe.h>
#include <string>
#include <iostream>
#include "psql.h"


void CloseConn(PGconn *conn){
	PQfinish(conn);
	//getchar();
}

PGconn *ConnectDB(string user="postgres",string password="123321",string dbname="bridge",string hostaddr="127.0.0.1",string port="5432"){
	PGconn *conn = NULL;
	string s = "user=" + user + " password=" + password + " dbname=" +dbname + " hostaddr=" + hostaddr + " port=" + port;

	// Make a connection to the database
	conn = PQconnectdb(s.c_str());
	
	// Check to see that the backend connection was successfully made
	if (PQstatus(conn) != CONNECTION_OK){
		cout << "Connection to database failed.\n";
		CloseConn(conn);
	}

	cout << "Connection to database - OK\n";

	return conn;
}


int login_check(PGconn *conn, string username, string password){
	string query = "SELECT * FROM login where plid='" + username + "'";

	PGresult *res = PQexec(conn,query.c_str());

	if(PQresultStatus(res) == PGRES_TUPLES_OK){ //successful completion of a command returning data
		cout << "query executed successfully\n";
		int row = PQntuples(res); // number of rows in the output of the query
		cout<<row<<endl;
		if (row != 1){
			//wrong username
		}
		else{      
			//cout<<PQgetvalue(res,0,1)<<endl;
			//cout<<password<<endl;
			if( !(string(PQgetvalue(res,0,1)).compare(password)) ){ //return 0 on equality
				cout<<"valid user";
				PQclear(res);
				return 0;
			}
		}
	}

	// Clear result
	PQclear(res);
	return -1;
}