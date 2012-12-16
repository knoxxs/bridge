#include <math.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include "myregex.h"
#include "access.h"



void waitProc(char* id_proc){

    char command[6 + strlen(id_proc)];

    logp("myregex-waitProc",0,0,"Start making /proc string");
    strcpy(command, "/proc/");
    strcat(command, id_proc);
    logp("myregex-waitProc",0,0,"Done making proc string");

    while(open(command, O_DIRECTORY) != -1 ) {
        logp("myregex-waitProc",0,0,"Waiting for process to exit");
        sleep(0.2);
    }

    logp("myregex-waitProc",0,0,"Process Exited Completly");
}

void read_command_output(char* command, char* buf, int buf_len){
    
    FILE *ps_output_file;
    if ((ps_output_file = popen(command,"r"))== NULL){
        errorp("myregex-read_command_output",0,0,"Error while executing the command (popen)");
        exit(1);
    }

    if(fgets(buf, buf_len, ps_output_file)==NULL){
        errorp("myregex-read_command_output",0,0,"Reading the command ooutput");
    }    

    if(fclose(ps_output_file) != 0){
        errorp("myregex-read_command_output",0,0,"Unable to close the command output file");
        debugp("myregex-read_command_output",1,errno,NULL);
    }

    return;
}

void compileRegex(regex_t* regexPtr, char* pattern){
    int errcode;
    char buf[30];
    if( (errcode = regcomp(regexPtr, pattern, REG_EXTENDED)) != 0 ){ 
        errorp("myregex-compileRegex",0,0,"Unable to Compile the regex");
        sprintf(buf,"Error Code is %d",errcode);
        debugp("myregex-processIdFinder",0,0,buf);
        exit(1); 
    }
}

int executeRegex(regex_t* regex, char* buf, size_t nmatch, regmatch_t* reg_matches, int flags){

    char errmsgbuf[100];
    int err_ret;
    logp("myregex-executeRegex",0,0,"Calling regexec");
    err_ret = regexec(regex, buf, nmatch, reg_matches, flags);
    if( !err_ret ){
        logp("myregex-executeRegex",0,0,"regex match found");
        return 0;    
    }
    else if( err_ret == REG_NOMATCH ){
        logp("myregex-executeRegex",0,0,"NO regex match found");
        return -1;
    }
    else{
        regerror(err_ret, regex, errmsgbuf, sizeof(errmsgbuf));
        errorp("myregex-executeRegex",0,0,"regex match FAILED");
        debugp("myregex-executeRegex",0,0,errmsgbuf);
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

    char buf[CMD_OUTPUT_BUF_SIZE], id_proc[MAX_PROC_ID_SIZE];
    regex_t regex;
    regmatch_t reg_matches[2];
    int id_len, ret, proc_id;



    char command[] = "/bin/ps -C ", commandGrep[] = " | grep ";
    char completeCommand[ strlen(command) + strlen(commandGrep) + 2 * MAX_PROC_NAME_SIZE];
    //"/bin/ps -C supervisor | grep supervisor"

    logp("myregex-processIdFinder",0,0,"Start making the command string");
    strcpy(completeCommand, command);
    strcat(completeCommand, commandName);
    strcat(completeCommand, commandGrep);
    strcat(completeCommand, commandName);//strcat also add the null terminate byte
    logp("myregex-processIdFinder",0,0,"Command string made succesfully");

    logp("myregex-processIdFinder",0,0,"Calling read_command_output");
    read_command_output(completeCommand, buf, CMD_OUTPUT_BUF_SIZE);
    
    logp("myregex-processIdFinder",0,0,"Calling compileRegex");
    compileRegex(&regex, "^ *([0-9]+) ?");

    logp("myregex-processIdFinder",0,0,"Calling executeRegex");
    ret = executeRegex(&regex, buf, (size_t) 2, reg_matches, 0);
    
    if(ret == 0){

        logp("myregex-processIdFinder",0,0,"Fetching matched result");
        //fetching result
        id_len = reg_matches[1].rm_eo - reg_matches[1].rm_so; //eo is index of last +1
        //printf("%d and %d  ----- %c and %c\n",reg_matches[1].rm_so,reg_matches[1].rm_eo,buf[reg_matches[1].rm_so],buf[reg_matches[1].rm_eo]);
        
        strncpy(id_proc,buf+reg_matches[1].rm_so, id_len);
        id_proc[id_len] = '\0';
        debugp("myregex-processIdFinder",0,0,"Process ID - ");
        debugp("myregex-processIdFinder",0,0,id_proc);
        
        logp("myregex-processIdFinder",0,0,"Calling strintToInt");
        proc_id = strintToInt(id_proc,strlen(id_proc));//its important to write strlen here not 10
        //printf("%d\n", proc_id); 

        logp("myregex-processIdFinder",0,0,"Returning proc_id");
        return proc_id;

    } else if(ret == -1){//no match
        return -1;
        //TODO need to also do somethng here
    } else if (ret == -2){//error
        return -1;
        //TODO need to also do somethng here
    }
}

int clearPort( char* portNo){

    char buf[100], id_proc[10];
    regex_t regex;
    regmatch_t reg_matches[2];
    int id_len, ret, proc_id;

    char command[] = "/bin/netstat -nlp | grep ";
    char completeCommand[ strlen(command) + 4 + 1 ]; 
    
    logp("myregex-clearPort",0,0,"making the Command string");
    strcpy(completeCommand, command);
    strcat(completeCommand, portNo);//strcat also add the null terminate byte
    logp("myregex-clearPort",0,0,"Command String made Successfully");

    logp("myregex-clearPort",0,0,"Calling read_command_output");
    read_command_output(completeCommand, buf, 100);

    logp("myregex-clearPort",0,0,"Calling compileRegex");
    compileRegex(&regex, "([0-9]+)/");

    logp("myregex-clearPort",0,0,"Calling executeRegex");
    executeRegex(&regex, buf, (size_t) 2, reg_matches, 0);

    if(ret == 0){
        logp("myregex-clearPort",0,0,"Fetching Matched Result");
        //fetching result
        id_len = reg_matches[1].rm_eo - reg_matches[1].rm_so; //eo is index of last +1
        //printf("%d and %d  ----- %c and %c\n",reg_matches[1].rm_so,reg_matches[1].rm_eo,buf[reg_matches[1].rm_so],buf[reg_matches[1].rm_eo]);
        
        strncpy(id_proc,buf+reg_matches[1].rm_so, id_len);
        id_proc[id_len] = '\0';
        logp("myregex-clearPort",0,0,"Process Id is");
        logp("myregex-clearPort",0,0,id_proc);

        logp("myregex-clearPort",0,0,"Calling strintToInt");
        proc_id = strintToInt(id_proc,strlen(id_proc));
        //printf("%d\n", proc_id); //its important to write strlen here not 10
        
        if(proc_id > 0){
            if(kill( (pid_t) proc_id, SIGKILL) == 0){
                logp("myregex-clearPort",0,0,"Process was killed Succesfully");
                logp("myregex-clearPort",0,0,"Start waiting for its exit");
                waitProc(id_proc);
                return 0;
            }
            else{
                logp("myregex-clearPort",0,0,"Killing the process");
                debugp("myregex-clearPort",1,errno,NULL);
                return -1;
            }
        } 
        else{
            logp("myregex-clearPort",0,0,"Proc ID is less than 0");
            return -1;
        }
    }
    else if(ret == -1){//no match
        return -1;
        //TODO need to do something
    } else if (ret == -2){//error
        return -1;
        //TODO need to do something
    }
}

// void regtest(){

//     char buf[100] = "bbbbbbbbbbb aaa bbbbbbbbbb", id_proc[10];
//     regex_t regex;
//     regmatch_t reg_matches[2];
//     int id_len, ret;

//     compileRegex(&regex, " (aaa) ");

//     executeRegex(&regex, buf, (size_t) 2, reg_matches, 0);
    
//     if(ret == 0){
//         //fetching result
//         id_len = reg_matches[1].rm_eo - reg_matches[1].rm_so; //eo is index of last +1
//         //printf("%d and %d  ----- %c and %c\n",reg_matches[1].rm_so,reg_matches[1].rm_eo,buf[reg_matches[1].rm_so],buf[reg_matches[1].rm_eo]);
//         strncpy(id_proc,buf+reg_matches[1].rm_so, id_len);
//         id_proc[id_len] = '\0';
//         printf("(%s)\n",id_proc);
//         printf("%d\n",strintToInt(id_proc,strlen(id_proc))); //its important to write strlen here not 10

//         return;
//     } else if(ret == -1){//no match
//         return;
//         //TODO need to log that unnable to find the process
//     } else if (ret == -2){//error
//         return;
//         //TODO need to log error
//     }
// }


//#define LOG_PATH "/home/abhi/Projects/bridge/server/log"

// int main() 
// {   
//         int logfile;

//     setLogFile(STDOUT_FILENO);
//     if( (logfile = open(LOG_PATH, O_RDWR|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)) < 0 ){
//         errorp("Supervisor -Main",0,0,"Error Opening logfile");
//         debugp("Supervisor -Main",1,1,NULL);
//     }
//     setLogFile(logfile);

//     processIdFinder("supervisor");
//     clearPort("4444");
//     //regtest();
// //    waitProc("9999");
//     return 0;
// }
