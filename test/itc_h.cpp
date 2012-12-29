#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <iostream>

#define MSGSZ     128

struct msgbuff {
long    mtype;
int a;
};

int msqid;
int msgflg = IPC_CREAT | 0666;
key_t key;

void *f1(int *x), *f2(int* x);

int main()  
{
  pthread_t f2_thread, f1_thread; 
  
   
    key = 1234;
    if ((msqid = msgget(key, msgflg )) < 0) {
      printf("msg queue not created");
        
    }
    else 
    printf("msgget: msgget succeeded: %d",msqid);

  int i1,i2;
  i1 = 1;
  i2 = 2;
  pthread_create(&f1_thread,NULL,f1,(void *)&i1);
  pthread_create(&f2_thread,NULL,f2,(void *)&i2);
  pthread_join(f1_thread,NULL);
  pthread_join(f2_thread,NULL);
  return 0;
}

void *f1(int *x){
  int i;
  i = *x;
  msgbuff sbuf;
  sbuf.mtype = 1;
  sbuf.a = i;

  if (msgsnd(msqid, &sbuf, 1, IPC_NOWAIT) < 0) {
       printf ("%d, %d, %d\n", msqid, sbuf.mtype, sbuf.a);
       printf("error sending error");
       
      pthread_exit(0); 
}
}

void *f2(int *x){
  int i;
  i = *x;
  msgbuff  rbuf;
  if (msgrcv(msqid, &rbuf, MSGSZ, 1, 0) < 0) {
        printf("error recving");
        
    }

    printf("recievind data is %d\n",rbuf.a);
  pthread_exit(0); 
}
