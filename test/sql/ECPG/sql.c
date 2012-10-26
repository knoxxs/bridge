/* Processed by ecpg (4.7.0) */
/* These include files are added by the preprocessor */
#include <ecpglib.h>
#include <ecpgerrno.h>
#include <sqlca.h>
/* End of automatic include section */

#line 1 "sql.pgc"
	#include<stdio.h>


	int main()
	{


	    { ECPGconnect(__LINE__, 0, "testdb@localhost:5432" , "'postgres'" , "'123321'" , NULL, 0); }
#line 8 "sql.pgc"


	    { ECPGdo(__LINE__, 0, 1, NULL, 0, ECPGst_normal, "insert into player1 values ( 1 , 'aam' , 'a' )", ECPGt_EOIT, ECPGt_EORT);}
#line 10 "sql.pgc"


	    { ECPGtrans(__LINE__, NULL, "commit");}
#line 12 "sql.pgc"


	    { ECPGdisconnect(__LINE__, "database");}
#line 14 "sql.pgc"


	    return 0;
	}