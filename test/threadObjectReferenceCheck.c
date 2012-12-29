#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

void* handler(void*);
void* handler2(void*);


struct gameThreadArg{
    char* a;
};


int main(){
    //creating thread of the player
    pthread_t thread_id = 0;
    //void* thread_arg_v;
    int err;

    struct gameThreadArg thread_arg;//structure declared at top
    thread_arg.a = NULL;

    if((err = pthread_create(&thread_id, NULL, handler, &thread_arg ))!=0) {
        printf("%d\n",err );
    }

    printf("%d\n",err );
    if ((err = pthread_detach(thread_id)) != 0) {
        printf("%d\n",err );
    }

    sleep(15);

    return;
}

void* handler(void* arg){
    //creating thread of the player
    pthread_t thread_id = 0;
    //void* thread_arg_v;
    int err;
    char str[] = "testing";

    struct gameThreadArg thread_arg;//structure declared at top
    thread_arg.a = str;

    printf("%s\n",str );
    if((err = pthread_create(&thread_id, NULL, handler2, &thread_arg ))!=0) {
        printf("%d\n",err );
    }

    printf("%d\n",err );
    if ((err = pthread_detach(thread_id)) != 0) {
        printf("%d\n",err );
    }

    sleep(3);
    return;
}

void* handler2(void* arg){
    struct gameThreadArg playerInfo;
    playerInfo = *( (struct gameThreadArg*) (arg) );

    char *str;

    printf("%s\n","err1" );

    str = playerInfo.a;

    printf("%s\n",str);

    sleep(6);

    printf("%s\n","err" );
    printf("%s\n",str);
    return;
}