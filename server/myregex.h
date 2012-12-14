#ifndef myregex
#define myregex

#include<regex.h>

#define MAX_PROC_ID_SIZE 10
#define MAX_PROC_NAME_SIZE 12
#define CMD_OUTPUT_BUF_SIZE 100


int strintToInt(char *str, int len);
void read_command_output(char*, char*, int);
int processIdFinder(char*);
int clearPort(char*);
void compileRegex(regex_t*, char*);
int executeRegex(regex_t*, char*, size_t, regmatch_t*, int);
void regtest();
void waitProc(char*);

#endif