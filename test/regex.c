#include <math.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <regex.h>

int strintToInt(char *str, int len);
void read_command_output(char*, char*, int);
void processIdFinder(char*);
void portClearer(char*);
void compileRegex(regex_t*, char*);
void executeRegex(regex_t*, char*, size_t, regmatch_t*, int);
void regtest();
int main()
{   
    processIdFinder("supervisor");
    portClearer("4444");
    //regtest();
    return 0;
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

void executeRegex(regex_t* regex, char* buf, size_t nmatch, regmatch_t* reg_matches, int flags){

    char errmsgbuf[100];
    int err_ret;
    err_ret = regexec(regex, buf, nmatch, reg_matches, flags);
    if( !err_ret ){
        //puts("Match");
        return;    
    }
    else if( err_ret == REG_NOMATCH ){
            puts("No match");
            exit(1);
    }
    else{
            regerror(err_ret, regex, errmsgbuf, sizeof(errmsgbuf));
            fprintf(stderr, "Regex match failed: %s\n", errmsgbuf);
            exit(1);
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

void processIdFinder(char* commandName){

    char buf[100], id_proc[10];
    regex_t regex;
    regmatch_t reg_matches[2];
    int id_len;

    char command[] = "/bin/ps -C ", commandGrep[] = " | grep ";
    char completeCommand[ strlen(command) + strlen(commandGrep) + 2 * 12];
    //"/bin/ps -C supervisor | grep supervisor"

    strcpy(completeCommand, command);
    strcat(completeCommand, commandName);
    strcat(completeCommand, commandGrep);
    strcat(completeCommand, commandName);//strcat also add the null terminate byte

    read_command_output(completeCommand, buf, 100);
    
    compileRegex(&regex, "^ *([0-9]+) ?");

    executeRegex(&regex, buf, (size_t) 2, reg_matches, 0);
    
    //fetching result
    id_len = reg_matches[1].rm_eo - reg_matches[1].rm_so; //eo is index of last +1
    //printf("%d and %d  ----- %c and %c\n",reg_matches[1].rm_so,reg_matches[1].rm_eo,buf[reg_matches[1].rm_so],buf[reg_matches[1].rm_eo]);
    strncpy(id_proc,buf+reg_matches[1].rm_so, id_len);
    id_proc[id_len] = '\0';
    printf("(%s)\n",id_proc);
    printf("%d\n",strintToInt(id_proc,strlen(id_proc))); //its important to write strlen here not 10

    return;
}

void portClearer( char* portNo){

    char buf[100], id_proc[10];
    regex_t regex;
    regmatch_t reg_matches[2];
    int id_len;

    char command[] = "/bin/netstat -nlp | grep ";
    char completeCommand[ strlen(command) + 4 + 1 ]; 
    
    strcpy(completeCommand, command);
    strcat(completeCommand, portNo);//strcat also add the null terminate byte

    read_command_output(completeCommand, buf, 100);

    compileRegex(&regex, "([0-9]+)/");

    executeRegex(&regex, buf, (size_t) 2, reg_matches, 0);

    //fetching result
    id_len = reg_matches[1].rm_eo - reg_matches[1].rm_so; //eo is index of last +1
    //printf("%d and %d  ----- %c and %c \n",reg_matches[1].rm_so,reg_matches[1].rm_eo,buf[reg_matches[1].rm_so],buf[reg_matches[1].rm_eo]);
    strncpy(id_proc,buf+reg_matches[1].rm_so, id_len);
    id_proc[id_len] = '\0';
    printf("(%s)\n",id_proc);
    printf("%d\n",strintToInt(id_proc,strlen(id_proc))); //its important to write strlen here not 10

    return;
}

void regtest(){

    char buf[100] = "bbbbbbbbbbb aaa bbbbbbbbbb", id_proc[10];
    regex_t regex;
    regmatch_t reg_matches[2];
    int id_len;

    compileRegex(&regex, " (aaa) ");

    executeRegex(&regex, buf, (size_t) 2, reg_matches, 0);
    
    //fetching result
    id_len = reg_matches[1].rm_eo - reg_matches[1].rm_so; //eo is index of last +1
    printf("%d and %d  ----- %c and %c\n",reg_matches[1].rm_so,reg_matches[1].rm_eo,buf[reg_matches[1].rm_so],buf[reg_matches[1].rm_eo]);
    strncpy(id_proc,buf+reg_matches[1].rm_so, id_len);
    id_proc[id_len + 1] = '\0';
    printf("(%s)\n",id_proc);
//    printf("%d\n",strintToInt(id_proc,strlen(id_proc))); //its important to write strlen here not 10


}