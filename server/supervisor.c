#include "accessories.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>


#define LOG_PATH "./log"
//#define CUR_WORKING_DIREC "/home/abhi/Projects/bridge/server"
#define CUR_WORKING_DIREC "/home/vidisha/bridge/server"
//TODO different computer working directory issue
//TODO error handling of signalHandlers

void signalHandlerChild(int signal);
void signalHandlerSuper(int signal);
void makeDaemon(int);
void initSignalHandlers(struct sigaction*, struct sigaction*);


int main() { 
	int logfile;

	setLogFile(STDOUT_FILENO);
	logp("Supervisor- Main", 0,0 ,"Starting");
	if( (logfile = open(LOG_PATH, O_RDWR|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)) < 0 ){
		errorp("Supervisor -Main",0,0,"Error Opening logfile");
		debugp("Supervisor -Main",1,1,NULL);
	}
	setLogFile(logfile);
	logp("________________________",0,0,"________________________");

	logp("Supervisor- Main",0,0,"Logfile opened Succesfuly");



	logp("Supervisor- Main",0,0,"Making Daemon");
	makeDaemon(logfile);
	logp("Supervisor- Main",0,0,"Daemon Made Succesfuly");


	pid_t supervisor_pid, dispactcher_pid;

	//signal struct
	static struct sigaction sigact_struct_child, sigact_struct_super;
	
	logp("Supervisor- Main",0,0,"Starting Initialising Signal Handlers");
	initSignalHandlers(&sigact_struct_child, &sigact_struct_super);
	logp("Supervisor- Main",0,0,"Signal Initialised Succesfuly");

	//TODO pid int to string
	supervisor_pid = getpid();
	logp("Supervisor- Main",0,0,"Supervisor Pid is ");


	//Starting Player
	logp("Supervisor- Main",0,0,"Forking For Starting Player");
	dispactcher_pid = fork();
	if(dispactcher_pid < 0){ //parent
		errorp("Supervisor-Main",0,0,"Forking For Player");
    	debugp("Supervisor-Main",1,errno,NULL);
		_exit(127);
	}
	if(dispactcher_pid == 0){ //child
		logp("Supervisor- Main",0,0,"Executing Player");
		execl("./bin/player",(const char*) NULL,(char *) 0); // need to change this to exec + e - controll environment
		//only returns when error
		errorp("Supervisor-Main",0,0,"Executing Player");
    	debugp("Supervisor-Main",1,errno,NULL);
		_exit(127);
	}


	//Starting Dispatcher
	logp("Supervisor- Main",0,0,"Forking For Starting dispatcher");
	dispactcher_pid = fork();
	if(dispactcher_pid < 0){ //parent
		errorp("Supervisor-Main",0,0,"Forking For Dispatcher");
    	debugp("Supervisor-Main",1,errno,NULL);
		_exit(127);
	}
	if(dispactcher_pid == 0){ //child
		logp("Supervisor- Main",0,0,"Executing Dispatcher");
		execl("./bin/dispatcher",(const char*) NULL,(char *) 0); // need to change this to exec + e - controll environment
		//only returns when error
		errorp("Supervisor-Main",0,0,"Executing Dispatcher");
    	debugp("Supervisor-Main",1,errno,NULL);
		_exit(127);
	}

	logp("Supervisor- Main",0,0,"Entering Wait Loop");
	while(1){}
}

void initSignalHandlers(struct sigaction* sigact_struct_child,struct sigaction* sigact_struct_super){
	
	logp("Supervisor- initSignalHandlers",0,0,"Filling signal structs");
	sigfillset(&(sigact_struct_child->sa_mask));//add all the signals
	sigfillset(&(sigact_struct_super->sa_mask));//add all the signals

	logp("Supervisor- initSignalHandlers",0,0,"Setting Handlers of structs");
	sigact_struct_child->sa_handler = signalHandlerChild;
	sigact_struct_super->sa_handler = signalHandlerSuper;

	logp("Supervisor- initSignalHandlers",0,0,"Setting Flags");
	sigact_struct_child->sa_flags |= SA_RESTART;
	sigact_struct_super->sa_flags |= SA_RESTART;

	logp("Supervisor- initSignalHandlers",0,0,"Registering Child");
	//handling signal from child
	sigaction(SIGCHLD, sigact_struct_child, NULL);

	logp("Supervisor- initSignalHandlers",0,0,"Starting Registering Super");
	//signal handling for super process
	sigaction(SIGHUP,  sigact_struct_super, NULL); //hangup detected          /term    /1
	sigaction(SIGINT,  sigact_struct_super, NULL); //Interrupt from keyboard  /term    /2
	sigaction(SIGQUIT, sigact_struct_super, NULL); //Quit from keyboard      /core    /3
	sigaction(SIGILL,  sigact_struct_super, NULL); //Illegal Instruction      /core    /4
	sigaction(SIGABRT, sigact_struct_super, NULL); //Abort signal from abort /core    /6
	sigaction(SIGFPE,  sigact_struct_super, NULL); //Floating Pt. Exception  /core    /8
	//sigaction(SIGKILL, &sigact_struct_super, NULL); //Kill Signal           /term    /9
	sigaction(SIGSEGV, sigact_struct_super, NULL); //Invalid memory reference/core    /11
	sigaction(SIGPIPE, sigact_struct_super, NULL); //Broken Pipe: write to a pipe with no reader   /term   /13
	sigaction(SIGALRM, sigact_struct_super, NULL); //Timer signal from alarm /term    /14
	sigaction(SIGTERM, sigact_struct_super, NULL); //Termination signal      /term    /15
	sigaction(SIGUSR1, sigact_struct_super, NULL); //user defined signal 1   /term    /30,10,16
	sigaction(SIGUSR2, sigact_struct_super, NULL); //user defined signal 2   /term    /31,12,17
	//sigaction(SIGSTOP, &sigact_struct_super, NULL); //stop process          /stop    /17,19,23
	sigaction(SIGTSTP, sigact_struct_super, NULL); //stop typed at tty       /stop    /18,20,24
	sigaction(SIGTTIN, sigact_struct_super, NULL); //tty input from bg proc  /stop    /21,21,26
	sigaction(SIGTTOU, sigact_struct_super, NULL); //tty output from bg proc /stop    /22,22,27
	sigaction(SIGBUS,  sigact_struct_super, NULL); //bus error(bad mem access)/core    /10,7,10
	sigaction(SIGPROF, sigact_struct_super, NULL); //profiling time expired  /term    /27,27,29
	sigaction(SIGTRAP, sigact_struct_super, NULL); //trace/breakpoint trap   /core    /5
	sigaction(SIGVTALRM, sigact_struct_super, NULL); //virtual alrm clock    /term    /26,26,28
	sigaction(SIGXCPU, sigact_struct_super, NULL); //cpu time limit exceeded /core    /24,24,30
	sigaction(SIGXFSZ, sigact_struct_super, NULL); //file size limit exceeded/core    /25,25,31
	sigaction(SIGIOT,  sigact_struct_super, NULL); //IOT trap				  /core    /6
	logp("Supervisor- initSignalHandlers",0,0,"Done Registering Super");
}

void makeDaemon(int logfile){
	pid_t pidToDaemon, retPid;

	logp("Supervisor-makeDaemon",0,0,"Forking First Child");
	/* Fork off the parent process */
    pidToDaemon = fork();
    if (pidToDaemon < 0) {
    	errorp("Supervisor-makeDaemon",0,0,"Error While Forking First Child");
    	debugp("Supervisor-makeDaemon",1,errno,NULL);
        exit(EXIT_FAILURE);
    }
    if (pidToDaemon > 0) {
    	logp("Supervisor-makeDaemon",0,0,"Exiting First Parent");
        exit(EXIT_SUCCESS); 
    }

	/* -----------child (daemon) continues ----------------*/
    logp("Supervisor-makeDaemon",0,0,"First Child Continues and adding SIGHUP Handler");
    signal(SIGHUP,SIG_IGN);

	/* Fork off the SECOND parent process */
	logp("Supervisor-makeDaemon",0,0,"Forking Second Child");
    pidToDaemon = fork();
    if (pidToDaemon < 0) {
    	errorp("Supervisor-makeDaemon",0,0,"Error While Forking Second Child");
    	debugp("Supervisor-makeDaemon",1,errno,NULL);
        exit(EXIT_FAILURE);
    }
    if (pidToDaemon > 0) {
    	logp("Supervisor-makeDaemon",0,0,"Exiting Second Parent");
        exit(EXIT_SUCCESS); 
    }

    /* -----------SECOND child (daemon) continues ----------------*/
    logp("Supervisor-makeDaemon",0,0,"Second Child Continues and Creating New Session");

	/* Create a new SID for the child process */
	retPid = setsid();
	if (retPid < 0) {
    	errorp("Supervisor-makeDaemon",0,0,"Creating New Session");
    	debugp("Supervisor-makeDaemon",1,errno,NULL);
     	exit(EXIT_FAILURE);
	}

	logp("Supervisor-makeDaemon",0,0,"Changing umask");
	/* Change the file mode mask */
	umask(027);  //can also use umask(0), but this is better

	// Closing standard file descriptors 
	logp("Supervisor-makeDaemon",0,0,"Closing STDIN");
	close(STDIN_FILENO);
	// if (open("/dev/null",O_RDONLY) == -1) {
	// 	//die("failed to reopen stdin while daemonising (errno=%d)",errno);
	// }

	//uncomment when no log file------
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
	logp("Supervisor-makeDaemon",0,0,"Copying STDOUT/ERR to logfile");
	dup2(logfile,STDOUT_FILENO);
	dup2(logfile,STDERR_FILENO);
	

	/* Change the current working directory */
	//cant use short path "~/bridge/server"
	logp("Supervisor-makeDaemon",0,0,"Changing Current Working Directory");
	if ((chdir(CUR_WORKING_DIREC)) < 0) {
		errorp("Supervisor-makeDaemon",0,0,"Changing Directory");
        debugp("Supervisor- To check FORKING", 1, errno,NULL);
        exit(EXIT_FAILURE);
	}
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
			logWrite(4, log_pass);
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
			logWrite( 4 , log_pass);
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
