#include<unistd.h>
#include<stdlib.h>
#include<sys/types.h>
#include<stdio.h>
#include<string.h>

int main(){

    int pipefd[2];
    int ret=pipe(pipefd);
    if(ret==-1){
        perror("pipe");
        exit(1);
    }

    pid_t pid=fork();
    if(pid>0){
        printf("i am parent process,pid:%d\n",getpid());
        char buf[1024]={0};
        while(1){
            read(pipefd[0],buf,sizeof(buf));
            printf("Parent process received: %s,pid:%d\n", buf,getpid());

            char* str1="this is parent ";
            write(pipefd[1],str1,strlen(str1));
            //sleep(1);
        }
    }else if(pid==0){
        printf("i am child process,pid:%d\n",getpid());
        char buf1[1024]={0};
        while(1){
            char* str="hellohello";
            write(pipefd[1],str,strlen(str));
            //sleep(1);

            read(pipefd[0],buf1,sizeof(buf1));
            printf("Child process received: %s,pid:%d\n", buf1,getpid());
            bzero(buf1,sizeof(buf1));
    }
    }
    return 0;
}