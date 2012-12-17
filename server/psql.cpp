#include <libpq-fe.h>
#include <string>
#include <iostream>
#include "psql.h"
#include <string.h>
#include "access.h"


PGconn *conn = NULL;

void CloseConn(char* identity){
	char cmpltIdentity[CMPLT_IDENTITY_SIZE];
	strcpy(cmpltIdentity, identity);
	strcat(cmpltIdentity,"-CloseConn");

	logp(cmpltIdentity,0,0,"Closing the connection with the database");
	PQfinish(conn);
	logp(cmpltIdentity,0,0,"connection closed successfully");
}


int ConnectDB(string user="postgres",string password="123321",string dbname="bridge",string hostaddr="127.0.0.1",string port="5432", char* identity=NULL){
	char cmpltIdentity[CMPLT_IDENTITY_SIZE];
	strcpy(cmpltIdentity, identity);
	strcat(cmpltIdentity,"-ConnectDB");

	logp(cmpltIdentity,0,0,"Composing the connection string");
	string s = "user=" + user + " password=" + password + " dbname=" +dbname + " hostaddr=" + hostaddr + " port=" + port;

	// Make a connection to the database
	logp(cmpltIdentity,0,0,"Calling PGconnectdb");
	conn = PQconnectdb(s.c_str());
	
	// Check to see that the backend connection was successfully made
	logp(cmpltIdentity,0,0,"Checking whether the connection has been made");
	if (PQstatus(conn) != CONNECTION_OK){
		errorp(cmpltIdentity,0,0,"Unable to make the connection");
		debugp(cmpltIdentity,0,0,"Calling CloseConn");
		CloseConn(identity);
		conn = NULL;
		return -1;
	}

	logp(cmpltIdentity,0,0,"Connection to database is OK");
	return 0;
}


int login_check(string username, string password, char* identity){
	char cmpltIdentity[CMPLT_IDENTITY_SIZE], buf[100];
	strcpy(cmpltIdentity, identity);
	strcat(cmpltIdentity,"-login_check");

	logp(cmpltIdentity,0,0,"Making the query string");
	string query = "SELECT * FROM login where plid='" + username + "'";

	logp(cmpltIdentity,0,0,"Executing the query");
	PGresult *res = PQexec(conn,query.c_str());

	logp(cmpltIdentity,0,0,"Checking if the query executed successfully");
	if(PQresultStatus(res) == PGRES_TUPLES_OK){ //successful completion of a command returning data
		logp(cmpltIdentity,0,0,"Query executed successfully");

		logp(cmpltIdentity,0,0,"Fetching number of rows");
		int row = PQntuples(res); // number of rows in the output of the query		
		sprintf(buf,"Number of rows returned is %d", row);
		logp(cmpltIdentity,0,0,buf);

		if (row != 1){
			logp(cmpltIdentity,0,0,"Wrong username");
			
			logp(cmpltIdentity,0,0,"Clearing result");
			PQclear(res);
			return -1;

		}
		else{
			logp(cmpltIdentity,0,0,"Comparing password");
			if( !(string(PQgetvalue(res,0,1)).compare(password)) ){ //return 0 on equality
				logp(cmpltIdentity,0,0,"Valid login");
				
				logp(cmpltIdentity,0,0,"Clearing result");
				PQclear(res);
				return 0;
			}
		}
	}
	else {
		errorp(cmpltIdentity,0,0,"Error executing the query");
		return -2;
	}
}

int getPlayerInfoFromDb(string plid, char* name, char* team, char* identity){
	char cmpltIdentity[CMPLT_IDENTITY_SIZE], buf[100];
	strcpy(cmpltIdentity, identity);
	strcat(cmpltIdentity,"- getplayerInfoFromDb");

	logp(cmpltIdentity,0,0,"Composing query string");
    string query = "SELECT * FROM players where pid='" + plid + "'";

    logp(cmpltIdentity,0,0,"Executing query");
    PGresult *res = PQexec(conn,query.c_str());

    logp(cmpltIdentity,0,0,"Checking query execution");
    if(PQresultStatus(res) == PGRES_TUPLES_OK) { //successful completion of a command returning data
        logp(cmpltIdentity,0,0,"Query executed succesfully");

        logp(cmpltIdentity,0,0,"Checking number of rows");
        int row = PQntuples(res); // number of rows in the output of the query
        sprintf(buf,"Number of rows returned is %d", row);
		logp(cmpltIdentity,0,0,buf);

        if( row == 1){
            logp(cmpltIdentity,0,0,"Fetching data from the result");
            strcpy( name, PQgetvalue(res,0,2) );
            strcpy( team, PQgetvalue(res,0,1) );

            logp(cmpltIdentity,0,0,"Clearing result");
            PQclear(res);
            return 0;
        }
        else {
            errorp(cmpltIdentity,0,0,"Wrong number of rows retrieved");
            return -1;
        }
    }
    else {
        errorp(cmpltIdentity,0,0,"Error executing the query");
        return -2;
    }
}