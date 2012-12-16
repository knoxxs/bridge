#include <math.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>
#include "myregex.h"
#include <fcntl.h>
#include <sys/stat.h>

int strintToInt(char *str, int len);
void read_command_output(char*, char*, int);
int processIdFinder(char*);
int clearPort(char*);
void compileRegex(regex_t*, char*);
int executeRegex(regex_t*, char*, size_t, regmatch_t*, int);
void regtest();

// int main()
// {   
//  //   processIdFinder("supervisor");
// //    claerPort("4444");
//     //regtest();
//     waitProc("9999");
//     return 0;
// }

void waitProc(char* id_proc){

    char command[6 + strlen(id_proc)];

    strcpy(command, "/proc/");
    strcat(command, id_proc);

//    printf("%d\n", open(command, O_DIRECTORY));

    while(open(command, O_DIRECTORY) != -1 ) {
        sleep(0.2);
    }
}

void read_command_output(char* command, char* buf, int buf_len){
    
    FILE *ps_output_file;
    if ((ps_output_file = popen(command,"r"))== NULL){
        printf("error\n");
        exit(1);
    }

    if(fgets(buf, buf_len, ps_output_file)==NULL)
        printf("%s\n",buf);
    
    fclose(ps_output_file);

    return;
}

void compileRegex(regex_t* regexPtr, char* pattern){

    if( regcomp(regexPtr, pattern, REG_EXTENDED) ){ 
        fprintf(stderr, "Could not compile regex\n");
        exit(1); 
    }
}

int executeRegex(regex_t* regex, char* buf, size_t nmatch, regmatch_t* reg_matches, int flags){

    char errmsgbuf[100];
    int err_ret;
    err_ret = regexec(regex, buf, nmatch, reg_matches, flags);
    if( !err_ret ){
        //puts("Match");
        return 0;    
    }
    else if( err_ret == REG_NOMATCH ){
            printf("No match\n");
            return -1;    }
    else{
            regerror(err_ret, regex, errmsgbuf, sizeof(errmsgbuf));
            fprintf(stderr, "Regex match failed: %s\n", errmsgbuf);
            return -2;
    }
}



int strintToInt(char *str, int len)
{
    int iter, pow_iter, num=0;
    for(iter = 0, pow_iter = len-1; iter < len; iter++, pow_iter--)
    {
        num += (str[iter]-48)* ((int) pow(10,pow_iter));
    }
    return num;

}

int processIdFinder(char* commandName){

    char buf[100], id_proc[10];
    regex_t regex;
    regmatch_t reg_matches[2];
    int id_len, ret, proc_id;

    char command[] = "/bin/ps -C ", commandGrep[] = " | grep ";
    char completeCommand[ strlen(command) + strlen(commandGrep) + 2 * 12];
    //"/bin/ps -C supervisor | grep supervisor"

    strcpy(completeCommand, command);
    strcat(completeCommand, commandName);
    strcat(completeCommand, commandGrep);
    strcat(completeCommand, commandName);//strcat also add the null terminate byte

    read_command_output(completeCommand, buf, 100);
    
    compileRegex(&regex, "^ *([0-9]+) ?");

    ret = executeRegex(&regex, buf, (size_t) 2, reg_matches, 0);
    
    if(ret == 0){
        //fetching result
        id_len = reg_matches[1].rm_eo - reg_matches[1].rm_so; //eo is index of last +1
        //printf("%d and %d  ----- %c and %c\n",reg_matches[1].rm_so,reg_matches[1].rm_eo,buf[reg_matches[1].rm_so],buf[reg_matches[1].rm_eo]);
        strncpy(id_proc,buf+reg_matches[1].rm_so, id_len);
        id_proc[id_len] = '\0';
        printf("(%s)\n",id_proc);
        proc_id = strintToInt(id_proc,strlen(id_proc));
        printf("%d\n", proc_id); //its important to write strlen here not 10

        return proc_id;
    } else if(ret == -1){//no match
        return -1;
        //TODO need to log that unnable to find the process
    } else if (ret == -2){//error
        return -1;
        //TODO need to log error
    }
}

int clearPort( char* portNo){

    char buf[100], id_proc[10];
    regex_t regex;
    regmatch_t reg_matches[2];
    int id_len, ret, proc_id;

    char command[] = "/bin/netstat -nlp | grep ";
    char completeCommand[ strlen(command) + 4 + 1 ]; 
    
    strcpy(completeCommand, command);
    strcat(completeCommand, portNo);//strcat also add the null terminate byte

    read_command_output(completeCommand, buf, 100);

    compileRegex(&regex, "([0-9]+)/");

    executeRegex(&regex, buf, (size_t) 2, reg_matches, 0);

    if(ret == 0){
        //fetching result
        id_len = reg_matches[1].rm_eo - reg_matches[1].rm_so; //eo is index of last +1
        //printf("%d and %d  ----- %c and %c\n",reg_matches[1].rm_so,reg_matches[1].rm_eo,buf[reg_matches[1].rm_so],buf[reg_matches[1].rm_eo]);
        strncpy(id_proc,buf+reg_matches[1].rm_so, id_len);
        id_proc[id_len] = '\0';
        printf("(%s)\n",id_proc);
        proc_id = strintToInt(id_proc,strlen(id_proc));
        printf("%d\n", proc_id); //its important to write strlen here not 10
        if(proc_id > 0){
            if(!kill( (pid_t) proc_id, SIGKILL)){
                printf("%s %d and I am %d\n","Hey killing", proc_id, (int)getpid() );
                waitProc(id_proc);
                return 0;
            }
            else{
                printf("kill prob ID - %d errno - %d errmsg - %s id_len - %d\n", proc_id, errno, strerror(errno), id_len);
                return -1;
            }
        } else{
            printf("proc_id is less then 0\n");
            return -1;
        }
    } else if(ret == -1){//no match
        return -1;
        //TODO need to log that unnable to find the process
    } else if (ret == -2){//error
        return -1;
        //TODO need to log error
    }
}

void regtest(){

    char buf[100] = "bbbbbbbbbbb aaa bbbbbbbbbb", id_proc[10];
    regex_t regex;
    regmatch_t reg_matches[2];
    int id_len, ret;

    compileRegex(&regex, " (aaa) ");

    executeRegex(&regex, buf, (size_t) 2, reg_matches, 0);
    
    if(ret == 0){
        //fetching result
        id_len = reg_matches[1].rm_eo - reg_matches[1].rm_so; //eo is index of last +1
        //printf("%d and %d  ----- %c and %c\n",reg_matches[1].rm_so,reg_matches[1].rm_eo,buf[reg_matches[1].rm_so],buf[reg_matches[1].rm_eo]);
        strncpy(id_proc,buf+reg_matches[1].rm_so, id_len);
        id_proc[id_len] = '\0';
        printf("(%s)\n",id_proc);
        printf("%d\n",strintToInt(id_proc,strlen(id_proc))); //its important to write strlen here not 10

        return;
    } else if(ret == -1){//no match
        return;
        //TODO need to log that unnable to find the process
    } else if (ret == -2){//error
        return;
        //TODO need to log error
    }
}