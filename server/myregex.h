#ifndef regexFlag
#define regexFlag

#include<regex.h>

int strintToInt(char *str, int len);
void read_command_output(char*, char*, int);
int processIdFinder(char*);
int clearPort(char*);
void compileRegex(regex_t*, char*);
int executeRegex(regex_t*, char*, size_t, regmatch_t*, int);
void regtest();
void waitProc(char*);

#endif