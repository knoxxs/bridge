// Author: Al Dev alavoor@yahoo.com
//
// Program to monitor the unix processes
// and automatically re-start them if they die
//
//************************************************************************
//  NOTE: This program uses the Al Dev's String class library. Download string 
//        class from  http://linuxdoc.org/HOWTO/C++Programming-HOWTO.html
//************************************************************************


#include <stdio.h>
#include <strings.h> // C strings
#include <unistd.h> // for getopt
#include <alloc.h> // for free

#include <errno.h> // for kill() - error numbers command
extern int errno;

#ifdef Linux
#include <asm/errno.h> // for kill() - error numbers command
#endif

#include <sys/types.h> // for kill() command
#include <signal.h> // for kill() command
#include <sys/wait.h> // for wait()
#include <stdlib.h> // for setenv()
#include <time.h> // for strftime()
#include <libgen.h> // for basename()

//#include <syslog.h> // for logging 

#include "debug.h"
#include "String.h"
#include "StringTokenizer.h"

#define BUFF_HUN        100
#define BUFF_THOU       1024
#define PR_INIT_VAL     -10
#define WAIT_FOR_SYS    5  // wait for process to start up
#define DEF_SL_SECS     6  // default sleep time
#define SAFE_MEM        10  // to avoid any possible memory leaks

#define LOG_NO          false  // do not output to logfile
#define LOG_YES         true  // do output to logfile
#define STD_ERR_NO      false  // do not print to std err
#define STD_ERR_YES     true  // do print to std err
#define DATE_NO         false  // do not print date
#define DATE_YES        true  // do print date

int start_process(char *commandline, char *args[], char **envp, pid_t proc_pid);
int fork2(pid_t parent_pid, unsigned long tsecs);
inline void error_msg(char *mesg_out, char *lg_file, bool pr_lg, bool std_err, bool pr_dt);

//////////////////////////////////////////////
// To test this program use --
// procautostart -n 5 -c 'monitor_test dummy1 -a dummy2 -b dummy3  ' &
//////////////////////////////////////////////

void usage(char **argv)
{

    printf("%s:\n", argv[0]);
    printf("\ninterval specification:\n");
    printf(" -n # -- seconds\n");
    printf(" -m # -- microseconds\n");
    printf(" -o # -- nanoseconds\n");
    printf("\nprocess specification:\n");
    printf(" -c 'cmdline'\n");
    printf
        (" -p pidfile -- if specified reads process pid from this file\n");
    printf("\n");

        printf("\nUsage : %s -n <seconds> -m <microsecond> -o <nanosecond> -c '<command>'\n", argv[0]);
        printf("\nExample: procautostart -n 5 -c 'monitor_test dummy1 -a dummy2 -b dummy3  ' \n");

    exit(-1);
}

int main(int argc, char **argv, char **envp)
{
        unsigned long   sleep_sec, sleep_micro, sleep_nano;
        int     ch;
        pid_t   proc_pid;
        int pr_no = PR_INIT_VAL;
        char mon_log[40];
        char *pr_name = NULL, **cmdargs = NULL;
        String cmdline;
        char *pidfile = NULL;

        // you can turn on debug by editing Makefile and put -DDEBUG_PRT in gcc
        debug_("test debug", "this line"); 
        debug_("argc", argc); 

        // Use getpid() - man 2 getpid()
        proc_pid = getpid(); // get the Process ID of procautostart
        debug_("PID proc_pid", (int) proc_pid);

        // Create directory to hold log, temp files
        system("mkdir mon 1>/dev/null 2>/dev/null");

        sleep_sec = DEF_SL_SECS ; // default sleep time
        sleep_micro = 0; // default micro-sleep time
        sleep_nano = 0; // default nano-sleep time
        optarg = NULL;
        while ((ch = getopt(argc, argv, "n:m:o:h:c:")) != -1) // needs trailing colon :
        {
                switch (ch)
                {
                        case 'n':
                                debug_("scanned option n ", optarg);
                                sleep_sec = atoi(optarg);       
                                debug_("sleep_sec", sleep_sec);
                                break;
                        case 'm':
                                debug_("scanned option m ", optarg);
                                sleep_micro = atoi(optarg);     
                                debug_("sleep_micro", sleep_micro);
                                break;
                        case 'o':
                                debug_("scanned option o ", optarg);
                                sleep_nano = atoi(optarg);      
                                debug_("sleep_nano", sleep_nano);
                                break;
                        case 'c':
                                debug_("scanned option c ", optarg);
                                cmdline = optarg;
                                //cmdline = strdup(optarg); // does auto-malloc here
                                debug_("cmdline", cmdline.val());
                                break;
                        case 'h':
                                debug_("scanned option h ", optarg);
                                usage(argv);
                                break;
                        case 'p':
                                pidfile = strdup(optarg);
                                break;

                        default:
                                debug_("ch", "default");
                                usage(argv);
                                break;
                }
        }

        if (cmdline.length() == 0) //if (cmdline == NULL)
                usage(argv);

        // detach from the main process
    if (fork())  // 0 returned in child process
        exit(0); // exit parent process - non-zero child PID got here... 

    //openlog(argv[0], LOG_PID, LOG_DAEMON);

        // trim the trailing blanks -- otherwise problem in grep command
        //cmdline.trim(true);
        //cmdline.chopall('&'); // trim trailing ampersand
        debug_("cmdline", cmdline.val());

        // Start the process
        {
                // Find the command line args
                StringTokenizer strtokens(cmdline.val());  // string tokenizer is borrowed from Java
                cmdargs = (char **) calloc(strtokens.countTokens() + SAFE_MEM, sizeof(char *));
                debug_("countTokens()", strtokens.countTokens());
                for (int tmpii = 0; strtokens.hasMoreTokens(); tmpii++)
                {
                        cmdargs[tmpii] = strdup(strtokens.nextToken().val());
                        debug_("tmpii", tmpii);
                        debug_("cmdargs[tmpii]", (char *) cmdargs[tmpii]);
                }

                // In case execve you MUST NOT have trailing ampersand & in the command line!!
                //pr_no = start_process(cmdline, NULL, NULL, proc_pid);  // Using execlp ...
                pr_no = start_process(cmdargs[0], & cmdargs[0], envp, proc_pid); // Using execve ....
                //cmdpid = start_command(cmdargs, envp, pidfile);
                
                // You can also use syslog if you do not like above logging
                //syslog(LOG_NOTICE, "Started process: %s", cmdline.val());

                debug_("The child pid", pr_no);
                if (pr_no < 0)
                {
                        fprintf(stderr, "\nFatal Error: Failed to start the process\n");
                        exit(-1);
                }
                sleep(WAIT_FOR_SYS); // wait for the process to come up

                // Get process name - only the first word from cmdline
                pr_name = strdup(basename(cmdargs[0])); // process name,  does auto-malloc here
        }

        // generate log file names
        {
                char    aa[21];

                strncpy(aa, pr_name, 20); aa[20] = '\0';
                // Define mon file-names - make it unique with combination of
                // process name and process id
                sprintf(mon_log, "mon/%s%d.log", aa, (int) proc_pid);
        }

        // Print out pid to log file
        if (pr_no > 0)
        {
                char aa[200];
                sprintf(aa, "Process ID of %s is %d", pr_name, pr_no);
                error_msg(aa, mon_log, LOG_YES, STD_ERR_NO, DATE_YES);
        }

        // monitors the process - restarts if process dies...
        char print_log[200];
        while (1)  // infinite loop - monitor every 6 seconds
        {
                //debug_("Monitoring the process now...", ".");
        switch (kill(pr_no, 0))
        {
            case 0://process is running fine
                break;

            default:
            case ESRCH:    // process died !!
                                // ERSRCH means - No process can be found corresponding to pr_no
                                // hence process had died !!
                                sprintf(print_log, "Error ESRCH: No process or process group can be found for %d", pr_no);
                                error_msg(print_log, mon_log, LOG_YES, STD_ERR_YES, DATE_YES);
                                // You can also use syslog if you do not like above logging
                //syslog(LOG_NOTICE, "PROCESS DIED: %s", cmdline.val());
                                //pr_no = start_process(cmdline, NULL, NULL, proc_pid);  // Using execlp ....
                                pr_no = start_process(cmdargs[0], & cmdargs[0], envp, proc_pid); // Using execve ....
                                // You can also use syslog if you do not like below logging
                //syslog(LOG_NOTICE, "Started process: %s", cmdline.val());
                                sprintf(print_log, "Starting process %s", pr_name);
                                error_msg(print_log, mon_log, LOG_YES, STD_ERR_NO, DATE_NO);
                                sleep(WAIT_FOR_SYS); // wait for the process to come up
                break;
        }
                sprintf(print_log, "Process ID of %s is %d", pr_name, pr_no);
                error_msg(print_log, mon_log, LOG_YES, STD_ERR_NO, DATE_NO);
                //debug_("Sleeping now ......", ".");
                sleep(sleep_sec);

                // Uncomment these to use micro-seconds
                // For real-time process control use micro-seconds or nana-seconds sleep functions
                // See 'man3 usleep', 'man 2 nanasleep'
                // If you do not have usleep() or nanosleep() on your system, use select() or poll()
                // specifying no file descriptors to test.
                //usleep(sleep_micro);

                // To sleep nano-seconds ...  Uncomment these to use nano-seconds
                //struct timespec *req = new struct timespec;
                //req->tv_sec = 0; // seconds
                //req->tv_nsec = sleep_nano; // nanoseconds
                //nanosleep( (const struct timespec *)req, NULL);

                /* You can use select instead of sleep for portability
        struct timeval interval;
                interval.tv_sec += tmp;
                interval.tv_usec += (long int) 1e3 *tmp;
        select(1, NULL, NULL, NULL, & interval);
                */
        }
        //closelog(); if using syslog
}

inline void error_msg(char *mesg_out, char *lg_file, bool pr_lg, bool std_err, bool pr_dt)
{
        if (pr_lg) // (pr_lg == true) output to log file
        {
                char tmp_msg[BUFF_THOU];
                if (pr_dt == true) // print date and message to log file 'lg_file'
                {
                        sprintf(tmp_msg, "date >> %s; echo '\n%s\n' >> %s\n ", 
                                lg_file, mesg_out, lg_file);
                        system(tmp_msg);
                }
                else
                {
                        sprintf(tmp_msg, "echo '\n%s\n' >> %s\n ", 
                                mesg_out, lg_file);
                        system(tmp_msg);
                }
        }

        if (std_err) // (std_err == true) output to standard error
                fprintf(stderr, "\n%s\n", mesg_out);
        
        //debug_("mesg_out", mesg_out);
}

// start a process and returns PID or -ve value if error
// The main() function has envp arg as in - main(int argc, char *argv[], char **envp)
int start_process(char *commandline, char *args[], char **envp, pid_t parent_pid)
{
        int ff;
        unsigned long  tsecs;

        tsecs = time(NULL); // time in secs since Epoch 1 Jan 1970
        debug_("Time tsecs", tsecs); 

        // Use fork2() instead of fork to avoid zombie child processes
        switch (ff = fork2(parent_pid, tsecs))  // fork creates 2 process each executing the following lines
        {
        case -1:
                fprintf(stderr, "\nFatal Error: start_process() - Unable to fork process\n");
                _exit(errno);
                break;
        case 0:  // child2 process
                debug_("\nStarting the start child process\n", " ");
                // For child process to ignore the interrupts (i.e. to put
                // child process in "background" mode.
                // Signals are sent to all processes started from a 
                // particular terminal. Accordingly, when a program is to be run non-interactively
                // (started by &), the shell arranges that the program will ignore interrupts, so
                // it won't be stopped by interrupts intended for foreground processes.
                // Hence if previous value of signal is not IGN than set it to IGN.

                // Note: Signal handlers cannot be set for SIGKILL, SIGSTOP 
                if (signal(SIGINT, SIG_IGN) == SIG_ERR)
                        fprintf(stderr, "\nSignal Error: Not able to set signal to SIGINT\n");
                else
                if (signal(SIGINT, SIG_IGN) != SIG_IGN) // program already run in background
                        signal(SIGINT, SIG_IGN);  // ignore interrupts

                if (signal(SIGHUP, SIG_IGN) == SIG_ERR)
                        fprintf(stderr, "\nSignal Error: Not able to set signal to SIGHUP\n");
                else
                if (signal(SIGHUP, SIG_IGN) != SIG_IGN) // program already run in background
                        signal(SIGHUP, SIG_IGN);  // ignore hangups

                if (signal(SIGQUIT, SIG_IGN) == SIG_ERR)
                        fprintf(stderr, "\nSignal Error: Not able to set signal to SIGQUIT\n");
                else
                if (signal(SIGQUIT, SIG_IGN) != SIG_IGN) // program already run in background
                        signal(SIGQUIT, SIG_IGN);  // ignore Quit

                if (signal(SIGABRT, SIG_IGN) == SIG_ERR)
                        fprintf(stderr, "\nSignal Error: Not able to set signal to SIGABRT\n");
                else
                if (signal(SIGABRT, SIG_IGN) != SIG_IGN) // program already run in background
                        signal(SIGABRT, SIG_IGN);  // ignore ABRT

                if (signal(SIGTERM, SIG_IGN) == SIG_ERR)
                        fprintf(stderr, "\nSignal Error: Not able to set signal to SIGTERM\n");
                else
                if (signal(SIGTERM, SIG_IGN) != SIG_IGN) // program already run in background
                        signal(SIGTERM, SIG_IGN);  // ignore TERM

                // sigtstp - Stop typed at tty. Ignore this so that parent process 
                // be put in background with CTRL+Z or with SIGSTOP
                if (signal(SIGTSTP, SIG_IGN) == SIG_ERR)
                        fprintf(stderr, "\nSignal Error: Not able to set signal to SIGTSTP\n");
                else
                if (signal(SIGTSTP, SIG_IGN) != SIG_IGN) // program already run in background
                        signal(SIGTSTP, SIG_IGN);  // ignore TSTP

                // You can use debug_ generously because they do NOT increase program size!
                debug_("before execve commandline", commandline);
                debug_("before execve args[0]", args[0]);
                debug_("before execve args[1]", args[1]);
                debug_("before execve args[2]", args[2]);
                debug_("before execve args[3]", args[3]);
                debug_("before execve args[4]", args[4]);
                debug_("before execve args[5]", args[5]);
                debug_("before execve args[6]", args[6]);
                debug_("before execve args[7]", args[7]);
                debug_("before execve args[8]", args[8]);
                debug_("before execve args[9]", args[9]);
                execve(commandline, args, envp);

                // execlp, execvp does not provide expansion of metacharacters
                // like <, >, *, quotes, etc., in argument list. Invoke
                // the shell /bin/sh which then does all the work. Construct
                // a string 'commandline' that contains the complete command
                //execlp("/bin/sh", "sh", "-c", commandline, (char *) 0);  // if success than NEVER returns !!

                // If execlp returns than there is some serious error !! And
                // executes the following lines below...
                fprintf(stderr, "\nFatal Error: Unable to start child process\n");
                ff = -2;
                exit(127);
                break;
        default: // parent process i.e child0
                // child pid is ff;
                if (ff < 0)
                    fprintf(stderr, "\nFatal Error: Problem while starting child process\n");

                {
                    char    buff[BUFF_HUN];
                    FILE    *fp1;
                    sprintf(buff, "mon/%d%lu.out", (int) parent_pid, tsecs); // tsecs is unsigned long
                    fp1 = fopen(buff, "r");
                    if (fp1 != NULL)
                    {
                            buff[0] = '\0';
                            fgets(buff, BUFF_HUN, fp1);
                            ff = atoi(buff);
                    }
                    fclose(fp1);
                    debug_("start process(): ff - ", ff);
#ifndef DEBUG_PRT
                    sprintf(buff, "rm -f mon/%d%lu.out", (int) parent_pid, tsecs);
                    system(buff);
#endif // DEBUG_PRT
                }

                // define wait() to put child process in foreground or else put in background
                //waitpid(ff, & status, WNOHANG || WUNTRACED);
                //waitpid(ff, & status, WUNTRACED);
                //wait(& status); 

                break;
        }
        return ff;
}

/* fork2() -- like fork, but the new process is immediately orphaned
 *            (won't leave a zombie when it exits)
 * Returns 1 to the parent, not any meaningful pid.
 * The parent cannot wait() for the new process (it's unrelated).
 */
/* This version assumes that you *haven't* caught or ignored SIGCHLD. */
/* If you have, then you should just be using fork() instead anyway.  */

int fork2(pid_t parent_pid, unsigned long tsecs)
{
    pid_t mainpid, child_pid = -10;
    int status;
    char    buff[BUFF_HUN];

    if (!(mainpid = fork()))//child enters? ,child1 created
    {
        switch (child_pid = fork()) child 2 created whose parent is child1
        {
          case 0:// child2
                        //child_pid = getpid();
                        //debug_("At case 0 fork2 child_pid : ", child_pid);
                        return 0;
          case -1:
                        _exit(errno);    /* assumes all errnos are <256 */
          default: //child 1
                        debug_("fork2 child_pid : ", (int) child_pid);
                        sprintf(buff, "echo %d > mon/%d%lu.out", (int) child_pid, (int) parent_pid, tsecs);
                        system(buff);
                        _exit(0);
        }
    }

        //debug_("fork2 pid : ", pid);
    if (mainpid < 0 || waitpid(mainpid, & status, 0) < 0)
      return -1;

    if (WIFEXITED(status))
      if (WEXITSTATUS(status) == 0)
        return 1;
      else
        errno = WEXITSTATUS(status);
    else
      errno = EINTR;  /* well, sort of :-) */

    return -1;
}

//
// char respawn[1024];
// strcpy(respawn, cmdline);
// For "C" program use kill(pid_t process, int signal) function. 
// #include <signal.h> // See 'man 2 kill'  
// Returns 0 on success and -1 with errno set.
//              kill -0 $pid 2>/dev/null || respawn
// To get the exit return status do --
//              kill -0 $pid 2>/dev/null | echo $?
// Return value 0 is success and others mean failure
// Sending 0 does not do anything to target process, but it tests
// whether the process exists. The kill command will set its exit
// status based on this process.
//
// Alternatively, you can use
//              ps -p $pid >/dev/null 2>&1 || respawn
// To get the exit return status do --
//              ps -p $pid >/dev/null 2>&1 | echo $?
// Return value 0 is success and others mean failure


//***********************************************************************
//      You can use pidfile to get the process id
//**********************************************************************
/* 
void poll_pidfile(char *pidfile)
{
    struct stat buf;
    struct timeval interval =       
    {
        tv_sec:0, tv_usec:(long int) 1e4
    }; // 10 miliseconds

    while (stat(pidfile, & buf) ? errno == ENOENT : buf.st_size == 0)
    {
        struct timeval i = interval;
        select(1, NULL, NULL, NULL, & i);
    }
}

int start_command(char **args, char **envp, char *pidfile)
{
    pid_t cmdpid;

    if (pidfile != NULL)
    {
        switch (unlink(pidfile))
        {
            case ENOENT:
            case 0:
                break;

            default:
                return -1;
        }
    }

    switch (cmdpid = fork2())
    {
        case 0: // child
            execve(args[0], args, envp);
            exit(-1);

        case -1: // error
            return -1;

        default: // parent
            break;
    }

    if (pidfile != NULL)
    {
        FILE *pf;

        poll_pidfile(pidfile);

        if ((pf = fopen(pidfile, "r")) == NULL)
        {
            syslog(LOG_ERR, "failed to read pidfile, using fork2 pid");
        }
        else
        {
            char textpid[1024];
            pid_t pid;

            fgets(textpid, sizeof(textpid), pf);
            if ((pid = atoi(textpid)) != -1)
            {
                cmdpid = pid;
            } else
            syslog(LOG_ERR,
                    "failed to find pid in pidfile, using fork2 pid");
            fclose(pf);
        }
    }

    return cmdpid;
}
*/
