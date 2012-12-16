#include <iostream>
#include <libpq-fe.h>

//to-install - libpq-fe
//sudo apt-get install postgresql
//sudo apt-get install libpq-dev

using namespace std;
//sudo g++ PSQLTest.cpp -I /usr/include/postgresql -lpq -o PSQLTest
//sudo -u postgres psql #






/* Close connection to database */
void CloseConn(PGconn *conn)
{
  PQfinish(conn);
  getchar();
}





/* Establish connection to database */
PGconn *ConnectDB()
{
  PGconn *conn = NULL;

  // Make a connection to the database
  conn = PQconnectdb("user=postgres password=123321 dbname=testdb hostaddr=127.0.0.1 port=5432");

  // Check to see that the backend connection was successfully made
  if (PQstatus(conn) != CONNECTION_OK)
  {
    cout << "Connection to database failed.\n";
    CloseConn(conn);
  }

  cout << "Connection to database - OK\n";

  return conn;
}








/* Create employee table */
void CreateEmployeeTable(PGconn *conn)
{
  // Execute with sql statement
  PGresult *res = PQexec(conn, "CREATE TABLE employee (Fname char(30), Lname char(30))");
   
  if (PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    cout << "Create employee table failed\n";
    PQclear(res);
    CloseConn(conn);
  }

  cout << "Create employee table - OK\n";

  // Clear result
  PQclear(res);
}








/* Append SQL statement and insert record into employee table */
void InsertEmployeeRec(PGconn *conn, const char * fname, const char * lname)
{
  // Append the SQL statment
  std::string sSQL;
  sSQL.append("INSERT INTO employee VALUES ('");
  sSQL.append(fname);
  sSQL.append("', '");
  sSQL.append(lname);
  sSQL.append("')");
 
  // Execute with sql statement
  PGresult *res = PQexec(conn, sSQL.c_str());

  if (PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    cout << "Insert employee record failed\n";
    PQclear(res);
    CloseConn(conn);
  }

  cout << "Insert employee record - OK\n";

  // Clear result
  PQclear(res);
}










/* Fetch employee record and display it on screen */
void FetchEmployeeRec(PGconn *conn)
{
  // Will hold the number of field in employee table
  int nFields;

  // Start a transaction block
  PGresult *res  = PQexec(conn, "BEGIN");

  if (PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    cout << "BEGIN command failed\n";
    PQclear(res);
    CloseConn(conn);
  }

  // Clear result
  PQclear(res);

  // Fetch rows from employee table
  res = PQexec(conn, "DECLARE emprec CURSOR FOR select * from employee");
  if (PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    cout << "DECLARE CURSOR failed\n";
    PQclear(res);
    CloseConn(conn);
  }

  // Clear result
  PQclear(res);

  res = PQexec(conn, "FETCH ALL in emprec");

  if (PQresultStatus(res) != PGRES_TUPLES_OK)
  {
    cout << "FETCH ALL failed\n";
    PQclear(res);
    CloseConn(conn);
  }

  // Get the field name
  nFields = PQnfields(res);

  // Prepare the header with employee table field name
  cout << "\nFetch employee record:";
  cout << "\n********************************************************************\n";
    for (int i = 0; i < nFields; i++)
        printf("%-30s", PQfname(res, i));
    cout << "\n********************************************************************\n";

    // Next, print out the employee record for each row
    for (int i = 0; i < PQntuples(res); i++)
    {
        for (int j = 0; j < nFields; j++)
            printf("%-30s", PQgetvalue(res, i, j));
        printf("\n");
    }
 
    PQclear(res);

    // Close the emprec
    res = PQexec(conn, "CLOSE emprec");
    PQclear(res);

    // End the transaction
    res = PQexec(conn, "END");

  // Clear result
    PQclear(res);
}










/* Erase all record in employee table */
void RemoveAllEmployeeRec(PGconn *conn)
{
  // Execute with sql statement
  PGresult *res = PQexec(conn, "DELETE FROM employee");

  if (PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    cout << "Delete employees record failed.\n";
    PQclear(res);
    CloseConn(conn);
  }

  cout << "\nDelete employees record - OK\n";

  // Clear result
  PQclear(res);
}










/* Drop employee table from the database*/
void DropEmployeeTable(PGconn *conn)
{
  // Execute with sql statement
  PGresult *res = PQexec(conn, "DROP TABLE employee");

  if (PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    cout << "Drop employee table failed.\n";
    PQclear(res);
    CloseConn(conn);
  }

  cout << "Drop employee table - OK\n";

  // Clear result
  PQclear(res);
}














int main()
{
  PGconn *conn = NULL;
  
  conn = ConnectDB();
  
  if (conn != NULL)
  {
    CreateEmployeeTable(conn);
    InsertEmployeeRec(conn, "Mario", "Hewardt");
    InsertEmployeeRec(conn, "Daniel", "Pravat");
    FetchEmployeeRec(conn);

    cout << "\nPress ENTER to remove all records & table.....\n";
    getchar();
    
    RemoveAllEmployeeRec(conn);
    
    DropEmployeeTable(conn);
    
    CloseConn(conn);
  }

  return 0;
}