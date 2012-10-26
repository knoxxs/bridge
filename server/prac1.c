#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<unistd.h>

void sig_chld(int); 

int main()
{
    struct sigaction act;
    int i;
    act.sa_handler = sig_chld;
    pid_t pid;
    sigfillset(&act.sa_mask);
    

    if(sigaction(SIGCHLD, &act, NULL) < 0) 
        {
            fprintf(stderr, "sigaction failed\n");
            return -1;
       }

    switch(pid = fork())
        {
        case -1:
            {   fprintf(stderr, "fork failed\n");
               return 1;
        }    
        case 0:                        
             {   execl("./prac2",(const char*) NULL, (char *) 0);   
            exit(0);
        }
    default:
        {    
            for(i=0;i<10000000;i++);
            break;
        }
        }                       
                
    
    return 0;


}


void sig_chld(int signo) 
    {
            int status, child_val;
        if (waitpid(-1, &status, WNOHANG) < 0) 
            {
                  fprintf(stderr, "waitpid failed\n");
                return;
            }

  
            if (WIFEXITED(status))                /* did child exit normally? */
            {
                child_val = WEXITSTATUS(status); /* get child's exit status */
                printf("child's exited normally with status %d\n\n", child_val);
            }

    }
