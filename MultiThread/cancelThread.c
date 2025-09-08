#include<pthread.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>

void * callback(void * arg){
    printf("child thread id:%ld\n",pthread_self());
    for(int i=0;i<10;i++){
        printf("child:%d\n",i);
    }
    return NULL;
}

int main(){
    pthread_t tid;
    int ret=pthread_create(&tid,NULL,callback,NULL);
    if(ret!=0){
        char* errstr=strerror(ret);
        printf("Thread creation failed: %s\n", errstr);
    }

    pthread_cancel(tid);
    
    for(int i=0;i<10;i++){
        printf("%d\n",i);
    }

    printf("main thread id:%ld\n, tid is:%ld\n",pthread_self(),tid);

    pthread_exit(NULL);

    return 0;
}