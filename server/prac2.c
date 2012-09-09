#include <math.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <regex.h>

int strintToInt(char *str, int len);
int main()
{
	char buf_bind_kill[100],id_proc_to_kill[10];
	FILE *bind_kill_file;

	if ((bind_kill_file = popen("/bin/netstat -nlp | grep 32958","r"))== NULL)
	{
		printf("error\n");
		exit(1);
	}
	if(fgets(buf_bind_kill,100,bind_kill_file)==NULL)
		//error
	printf("%s\n",buf_bind_kill);
	fclose(bind_kill_file);
 
 	regex_t regex;
	regmatch_t reg_matches[2];
 	int err_ret;
    char msgbuf[100];

	/* Compile regular expression */
    err_ret = regcomp(&regex, "[1-9]+/", REG_EXTENDED);
    if( err_ret ){ 
    	fprintf(stderr, "Could not compile regex\n"); exit(1); 
    }

	/* Execute regular expression */
    err_ret = regexec(&regex, buf_bind_kill, (size_t) 2, reg_matches, 0);
        if( !err_ret ){
                //puts("Match");
                //printf("%d and %d  ----- %c and %c\n",matches[0].rm_so,matches[0].rm_eo,buf_bind_kill[matches[0].rm_so],buf_bind_kill[matches[0].rm_eo - 1]);
                strncpy(id_proc_to_kill,buf_bind_kill+reg_matches[0].rm_so, reg_matches[0].rm_eo - reg_matches[0].rm_so-1);
                printf("%s\n",id_proc_to_kill);
                printf("%d\n",strintToInt(id_proc_to_kill,3));
        }
        else if( err_ret == REG_NOMATCH ){
                puts("No match");
        }
        else{
                regerror(err_ret, &regex, msgbuf, sizeof(msgbuf));
                fprintf(stderr, "Regex match failed: %s\n", msgbuf);
                exit(1);
        }
    return 0;
}

