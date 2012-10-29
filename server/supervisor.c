#include "accessories.h"


void signalHandlerChild(int signal);
void signalHandlerSuper(int signal);

int main() { 

	pid_t pidToDaemon, retPid;


	/* Fork off the parent process */
    pidToDaemon = fork();
    if (pidToDaemon < 0) {
            exit(EXIT_FAILURE);
    }
    if (pidToDaemon > 0) {
            exit(EXIT_SUCCESS); 
    }
  
	/* -----------child (daemon) continues ----------------*/

    signal(SIGHUP,SIG_IGN);
	/* Fork off the SECOND parent process */
    pidToDaemon = fork();
    if (pidToDaemon < 0) {
            exit(EXIT_FAILURE);
    }
    if (pidToDaemon > 0) {
            exit(EXIT_SUCCESS); 
    }

    /* -----------SECOND child (daemon) continues ----------------*/

	/* Create a new SID for the child process */
	retPid = setsid();
	if (retPid < 0) {
    	 /* Log any failure */
     	exit(EXIT_FAILURE);
	}


	/* Change the file mode mask */
	umask(027);  //can also use umask(0), but this is better

	// Closing standard file descriptors ????????????????
	close(STDIN_FILENO);
	if (open("/dev/null",O_RDONLY) == -1) {
		//die("failed to reopen stdin while daemonising (errno=%d)",errno);
	}

	//no log file------
	/*
	close(STDOUT_FILENO);
	if (open("/dev/null",O_WRONLY) == -1) {
		//die("failed to reopen stdout while daemonising (errno=%d)",errno);
	}
	close(STDERR_FILENO);
	if (open("/dev/null",O_RDWR) == -1) {
		//die("failed to reopen stderr while daemonising (errno=%d)",errno);
	}
	*/

	//with log file
	int logfile_fileno = open("/home/abhi/Projects/bridge/SuperLog/log",O_RDWR|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
	if (logfile_fileno == -1) {
    	//die("failed to open logfile (errno=%d)",errno);
	}

	dup2(logfile_fileno,STDOUT_FILENO);
	dup2(logfile_fileno,STDERR_FILENO);
	
	write(logfile_fileno, "abhishek1\n",11);

//	close(logfile_fileno);


	/* Change the current working directory */
	//cant use short path "~/bridge/server"
	if ((chdir("/home/abhi/Projects/bridge/server")) < 0) {
        /* Log any failure here */
        errorp("Supervisor- To check FORKING", 1, errno,"UYYYYYY0");
        exit(EXIT_FAILURE);
	}

	write(logfile_fileno, "abhishek2\n",11);

	pid_t supervisor_pid, dispactcher_pid;
	int dispatcher_status;

	//signal struct
	static struct sigaction sigact_struct_child, sigact_struct_super;
	sigfillset(&(sigact_struct_child.sa_mask));//add all the signals
	sigfillset(&(sigact_struct_super.sa_mask));//add all the signals

	sigact_struct_child.sa_handler = signalHandlerChild;
	sigact_struct_super.sa_handler = signalHandlerSuper;

	sigact_struct_child.sa_flags |= SA_RESTART;
	sigact_struct_super.sa_flags |= SA_RESTART;

	//handling signal from child
	sigaction(SIGCHLD, &sigact_struct_child, NULL);

	write(logfile_fileno, "abhishek3\n",11);

	//signal handling for super process
	sigaction(SIGHUP, &sigact_struct_super, NULL); //hangup detected          /term    /1
	sigaction(SIGINT, &sigact_struct_super, NULL); //Interrupt from keyboard  /term    /2
	sigaction(SIGQUIT, &sigact_struct_super, NULL); //Quit from keyboard      /core    /3
	sigaction(SIGILL, &sigact_struct_super, NULL); //Illegal Instruction      /core    /4
	sigaction(SIGABRT, &sigact_struct_super, NULL); //Abort signal from abort /core    /6
	sigaction(SIGFPE, &sigact_struct_super, NULL); //Floating Pt. Exception  /core    /8
	//sigaction(SIGKILL, &sigact_struct_super, NULL); //Kill Signal           /term    /9
	sigaction(SIGSEGV, &sigact_struct_super, NULL); //Invalid memory reference/core    /11
	sigaction(SIGPIPE, &sigact_struct_super, NULL); //Broken Pipe: write to a pipe with no reader   /term   /13
	sigaction(SIGALRM, &sigact_struct_super, NULL); //Timer signal from alarm /term    /14
	sigaction(SIGTERM, &sigact_struct_super, NULL); //Termination signal      /term    /15
	sigaction(SIGUSR1, &sigact_struct_super, NULL); //user defined signal 1   /term    /30,10,16
	sigaction(SIGUSR2, &sigact_struct_super, NULL); //user defined signal 2   /term    /31,12,17
	//sigaction(SIGSTOP, &sigact_struct_super, NULL); //stop process          /stop    /17,19,23
	sigaction(SIGTSTP, &sigact_struct_super, NULL); //stop typed at tty       /stop    /18,20,24
	sigaction(SIGTTIN, &sigact_struct_super, NULL); //tty input from bg proc  /stop    /21,21,26
	sigaction(SIGTTOU, &sigact_struct_super, NULL); //tty output from bg proc /stop    /22,22,27
	sigaction(SIGBUS, &sigact_struct_super, NULL); //bus error(bad mem access)/core    /10,7,10
	sigaction(SIGPROF, &sigact_struct_super, NULL); //profiling time expired  /term    /27,27,29
	sigaction(SIGTRAP, &sigact_struct_super, NULL); //trace/breakpoint trap   /core    /5
	sigaction(SIGVTALRM, &sigact_struct_super, NULL); //virtual alrm clock    /term    /26,26,28
	sigaction(SIGXCPU, &sigact_struct_super, NULL); //cpu time limit exceeded /core    /24,24,30
	sigaction(SIGXFSZ, &sigact_struct_super, NULL); //file size limit exceeded/core    /25,25,31
	sigaction(SIGIOT, &sigact_struct_super, NULL); //IOT trap				  /core    /6
	


	supervisor_pid = getpid();

	write(logfile_fileno, "abhishek4\n",11);

	switch( (dispactcher_pid = fork()) )
	{
		case -1:
			{
				//error starting dispatcher child
				dispatcher_status = -1;
			}
		case 0: //dipatcher child
			{
					write(logfile_fileno, "abhishek5\n",11);
				printf("child %d\n",dispactcher_pid);
				execl("./bin/dispatcher",(const char*) NULL,(char *) 0); // need to change this to exec + e - controll environment
				_exit(127);
			}
	}



	int i;
	while(1)
	{}
//	for(i=0;i<1000000;i++);
//	printf("%d\n",supervisor_pid);
}


void signalHandlerChild(int signal){
	int status, return_val;
	pid_t pid;

	while ((pid = waitpid(-1, &status, WNOHANG)) > 0) 
	{
		//waitpid - return -1 on error, 0 if no child's state changed, on success return ID of the child whose state is changed
    	//here enters only if really a child is terminated.
    	//handle it.
    	if (WIFEXITED(status))                /* did child exit normally? */
            {
                return_val = WEXITSTATUS(status); /* get child's exit status */
                printf("child's exited normally with status %d\n\n", return_val);
            }
	}
}

void signalHandlerSuper(int signal)
{
	char pass[ strlen(PASSCHECK) ], temp[] = "User entered password- ",log_pass[ strlen(PASSCHECK) + strlen(temp) ];
	switch(signal)
	{
		case SIGHUP:
			errorp("Supervisor- signalHandlerSuper", 0, 0,"Caught signal SIGHUP");
			//TO-DO handle this exit code
			exit(9);
		case SIGINT:
			errorp("Supervisor- signalHandlerSuper", 0, 0,"Caught signal SIGINT");
			//authentication
			printf("Enter the passphrase to exit.\n");
			scanf("%s",pass);
			strcpy(log_pass,temp);
			strcat(log_pass, pass);
			logp(4, log_pass);
			if( strcmp(pass, PASSCHECK) == 0)
			{
				errorp("Supervisor- signalHandlerSuper in SIGINT", 0, 0,"User entered correct pass - so QUITING");
				//TO-DO handle any context to store
				//TO-DO handle this exit code
				exit(10);
			}
			else
			{
				errorp("Supervisor- signalHandlerSuper in SIGINT", 0, 0,"User entered wrong pass - so Continuing");
				printf("You entered wrong password. So bye.\n");
				return;
			}
			break;
		case SIGQUIT:
			errorp("Supervisor- signalHandlerSuper", 0, 0,"Caught signal SIGQUIT");
			//authentication
			printf("Enter the passphrase to exit.\n");
			scanf("%s",pass);
			strcpy(log_pass,temp);
			strcat(log_pass, pass);
			logp( 4 , log_pass);
			if( pass == PASSCHECK)
			{
				errorp("Supervisor- signalHandlerSuper in SIGQUIT", 0, 0,"User entered correct pass - so QUITING");
				//TO-DO handle any context to store
				//TO-DO handle this exit code
				exit(10);
			}
			else
			{
				errorp("Supervisor- signalHandlerSuper in SIGQUIT", 0, 0,"User entered wrong pass - so Continuing");
				printf("You entered wrong password. So bye.\n");
				return;
			}
			break;
		case SIGILL:
			errorp("Supervisor- signalHandlerSuper", 0, 0,"Caught signal SIGILL");
			//TO-DO handle this exit code and actual signal code
			exit(11);
		case SIGABRT:
			errorp("Supervisor- signalHandlerSuper", 0, 0,"Caught signal SIGABRT");
			//TO-DO handle this exit code
			exit(12);
		case SIGFPE:
			errorp("Supervisor- signalHandlerSuper", 0, 0,"Caught signal SIGFPE");
			//TO-DO handle this exit code
			exit(13);
		case SIGSEGV:
			errorp("Supervisor- signalHandlerSuper", 0, 0,"Caught signal SIGSEGV");
			//TO-DO handle this exit code
			exit(14);
		case SIGPIPE:
			errorp("Supervisor- signalHandlerSuper", 0, 0,"Caught signal SIGPIPE");
			//TO-DO handle this exit code
			exit(15);
		case SIGALRM:
			errorp("Supervisor- signalHandlerSuper", 0, 0,"Caught signal SIGALRM");
			//TO-DO handle this exit code
			exit(16);
		case SIGTERM:
			errorp("Supervisor- signalHandlerSuper", 0, 0,"Caught signal SIGTERM");
			//TO-DO handle this exit code
			exit(17);
		case SIGUSR1:
			errorp("Supervisor- signalHandlerSuper", 0, 0,"Caught signal SIGUSR1");
			//TO-DO handle this exit code
			exit(18);
		case SIGUSR2:
			errorp("Supervisor- signalHandlerSuper", 0, 0,"Caught signal SIGUSR2");
			//TO-DO handle this exit code
			exit(19);
		case SIGTSTP:
			errorp("Supervisor- signalHandlerSuper", 0, 0,"Caught signal SIGTSTP");
			//TO-DO handle this exit code
			exit(20);
		case SIGTTIN:
			errorp("Supervisor- signalHandlerSuper", 0, 0,"Caught signal SIGTTIN");
			//TO-DO handle this exit code
			exit(21);
		case SIGTTOU:
			errorp("Supervisor- signalHandlerSuper", 0, 0,"Caught signal SIGTTOU");
			//TO-DO handle this exit code
			exit(22);
		case SIGBUS:
			errorp("Supervisor- signalHandlerSuper", 0, 0,"Caught signal SIGBUS");
			//TO-DO handle this exit code
			exit(23);
		case SIGPROF:
			errorp("Supervisor- signalHandlerSuper", 0, 0,"Caught signal SIGPROF");
			//TO-DO handle this exit code
			exit(24);
		case SIGTRAP:
			errorp("Supervisor- signalHandlerSuper", 0, 0,"Caught signal SIGTRAP");
			//TO-DO handle this exit code
			exit(25);
		case SIGVTALRM:
			errorp("Supervisor- signalHandlerSuper", 0, 0,"Caught signal SIGVTALRM");
			//TO-DO handle this exit code
			exit(26);
		case SIGXCPU:
			errorp("Supervisor- signalHandlerSuper", 0, 0,"Caught signal SIGXCPU");
			//TO-DO handle this exit code
			exit(27);
		case SIGXFSZ:
			errorp("Supervisor- signalHandlerSuper", 0, 0,"Caught signal SIGXFSZ");
			//TO-DO handle this exit code
			exit(28);
		// case SIGIOT:
		// 	errorp("Supervisor- signalHandlerSuper", 0, 0,"Caught signal SIGIOT");
		// 	//TO-DO handle this exit code
		// 	exit(29);
	}
}