#include "access.h"
#include "psql.h"

void* SocketHandler(void* lp);
int listenBind(struct addrinfo *ai);
int clearPort();
void login(int fd);
//get sockaddr , IPv4 or IPv6:

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

int main(int argv, char** argc) {
    //printf(PORT);
    struct sockaddr_in my_addr;

    int listener; // listening the socket descriptor
    int newfd; //newly accept()ed file descriptor
    struct sockaddr_storage clientaddr; // client address
    socklen_t addrlen;
    void **thread_ret;

    char buf[256]; //buffer for client
    char remoteIP[INET6_ADDRSTRLEN];

    int i, j, rv;
    pthread_t thread_id = 0;
	void *thread_arg;
    struct addrinfo hints, *ai;
    int err;


    //get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        //fprintf(stderr, "selectserver : %s\n", gai_strerror(rv));
        errorp("DISPATCHER-getaddrinfo:", 1, rv, "a");
        exit(1);
    }
	logp(4,"struct addrinfo has been formed succesfully");
    
    listener = listenBind(ai);

   
    freeaddrinfo(ai); // all  done with this
	logp(4,"memory freed successfully");
	
    //listen
    if (listen(listener, 10) == -1) {
        errorp("DISPATCHER-listener:", 0, 0, "Unable to connect to reader");
        exit(3);
    }
    //Now lets do the server stuff
	logp(4,"socket is listening");
	logp(1,"socket is listening");

    while (1) {
    	
    	logp(1,"Waiting for connections");
    	
        if((newfd = accept(listener, (struct sockaddr *)&clientaddr, &addrlen)) == -1){
        	errorp("DISPATCHER-accept:", 0, 0, "Unable to accept one connection to reader");
			//continue;
		}

		logp(4,"accepted new connection");
		
		inet_ntop(clientaddr.ss_family, get_in_addr((struct sockaddr *)&clientaddr), remoteIP, INET6_ADDRSTRLEN);
		printf("server : got connection from %s\n", remoteIP);
		
		strcat(remoteIP,": Got new connection");
		logp(1,remoteIP);
		
		thread_arg=(void *)newfd;
		if((err = pthread_create(&thread_id, NULL, SocketHandler,thread_arg ))!=0)
		{
			errorp("DISPATCHER-pthrad_create:", 1, err, NULL);
		}
            
            if ((err = pthread_detach(thread_id)) != 0)
            {
            	errorp("DISPATCHER-pthread_detach:", 1, err, NULL);
            }
    }

    return 0;
}

void* SocketHandler(void* lp) {
	
    int fd = (long)lp;
	
	logp(1,"New thread created succesfully");
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
    // char buffer[6];
    // int buffer_len = 6;
    // int bytecount;
    // memset(buffer, 0, buffer_len);
    // if ((bytecount = recv(fd, buffer, buffer_len, 0)) == -1) {
    //     fprintf(stderr, "Error receiving data %d\n", errno);
    //     //goto FINISH;
    // }
    // printf("Received bytes %d\nReceived string \"%s\"\n", bytecount, buffer);
    // //strcat(buffer, "SERVER");
	
    // if ((bytecount = send(fd, buffer, strlen(buffer), 0)) == -1) {
    //     fprintf(stderr, "Error sending data %d\n", errno);
    //     //goto FINISH;
    // }
    // printf("Sent bytes %d\n", bytecount);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    char choice[1];
    int choice_len = sizeof(choice);
    int recvdBytes;
    if ((recvdBytes = recv(fd, choice, choice_len, 0)) == -1) {
        fprintf(stderr, "Error receiving data %d\n", errno);
    }
    //TODO - need to check if connection get closed.
    switch(choice[0])
    {
        case 'a':
            login(fd);
            break;
    }
	return ((void*)0);
}

void login(int fd)
{   
    char loginInfo[25], username[9], password[16];
    int loginInfo_len = sizeof(loginInfo);
    int recvdBytes;
    if ((recvdBytes = recv(fd, loginInfo, loginInfo_len, 0)) == -1) {
        fprintf(stderr, "Error receiving data %d\n", errno);
    }

    //printf("Hello Hello rcv(%d)\n%s \n",recvdBytes,loginInfo);
    
    sscanf(loginInfo, "%s%s",username, password);

    PGconn *conn = NULL;
    
    conn = ConnectDB("postgres","123321","bridge","127.0.0.1","5432");

    if (conn != NULL) {
        login_check(conn, username, password);
        CloseConn(conn);
    }
}

int clearPort()
{
    char buf_bind_kill[100], id_proc_to_kill[10], msgbuf[100];;
    FILE *bind_kill_file;
    regex_t regex;
    regmatch_t reg_matches[2];
    int err_ret, proc_id, id_len;

    if ((bind_kill_file = popen("/bin/netstat -nlp | grep 4444","r"))== NULL)
    {
        printf("error popen\n");
        exit(1);
    }
    if(fgets(buf_bind_kill,100,bind_kill_file)==NULL)
        printf("error fgets\n");//error

    fclose(bind_kill_file);
 
    /* Compile regular expression */
    err_ret = regcomp(&regex, "[0-9]+/", REG_EXTENDED);
    if( err_ret ){ 
        fprintf(stderr, "Could not compile regex\n");
        exit(1); 
    }

    /* Execute regular expression */
    err_ret = regexec(&regex, buf_bind_kill, (size_t) 2, reg_matches, 0);
        if( !err_ret ){
                //puts("Match");
                //printf("\n %s \n", buf_bind_kill);
                //printf("%d and %d  ----- %c and %c\n",reg_matches[0].rm_so,reg_matches[0].rm_eo,buf_bind_kill[reg_matches[0].rm_so],buf_bind_kill[reg_matches[0].rm_eo - 1]);
                id_len = reg_matches[0].rm_eo - reg_matches[0].rm_so-1;
                strncpy(id_proc_to_kill,buf_bind_kill+reg_matches[0].rm_so, id_len);
                //printf("%s\n",id_proc_to_kill);
                //printf("%d\n",strintToInt(id_proc_to_kill,3));
                proc_id = strintToInt(id_proc_to_kill,id_len);
                if(!kill( (pid_t) proc_id, SIGKILL))
                    return 0;
                else
                {
                    printf("kill prob ID - %d errno - %d errmsg - %s id_len - %d\n", proc_id, errno, strerror(errno), id_len);
                    return -1;
                }
        }
        else if( err_ret == REG_NOMATCH ){
                puts("No match");
                return 1;
        }
        else{
                regerror(err_ret, &regex, msgbuf, sizeof(msgbuf));
                fprintf(stderr, "Regex match failed: %s\n", msgbuf);
                return -1;
        }
    }

int listenBind(struct addrinfo *ai)
{
    struct addrinfo  *p;
    int yes = 1;
    int listener;

    for (p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) {
            continue;
        }
        logp(4,"socket formed succesfully");
        //lose the pesky "address already in use " error message
        //setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if ((setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int)))) {
            errorp("DISPATCHER-setsockopt:", 1, errno, NULL);
            //printf("Error setting options %d\n", errno);
            exit(1);
        }
        logp(4,"setsockopt formed succesfully");
        
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            errorp("DISPATCHER-bind:", 1, errno, "");
            continue;
        }
        logp(4,"binding has been done");
        break;
    }

    //if we got here, it means we didnot get bound
    if (p == NULL) {
        //fprintf(stderr, "selectserver: failed to bind\n");
        errorp("DISPATCHER-bindcheck:", 0, 0, "Unable to bind to port using any IP");
        if (errno == 98) //already in use
        {
            if (clearPort() == 0)
            {
                //printf("a\n");
                listener = listenBind(ai);
                return listener;
            }
            else if(clearPort() == 1)
            {    printf("error clearPort b\n");
                return -1;}
            else
                {printf("error clearPort c\n");
                exit(2);}
        }
    }
    return listener;
}
