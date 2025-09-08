#include<pthread.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>

void * callback(void * arg){
    printf("child thread id:%ld\n",pthread_self());
    return NULL;
}

int main(){
    pthread_t tid;
    int ret=pthread_create(&tid,NULL,callback,NULL);
    if(ret!=0){
        char* errstr=strerror(ret);
        printf("Thread creation failed: %s\n", errstr);
    }
    printf("main thread id:%ld\n, tid is:%ld\n",pthread_self(),tid);

    pthread_detach(tid);

    pthread_exit(NULL);

    return 0;
}