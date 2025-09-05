#include<unistd.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<stdio.h>
#include<string.h>

int main(){
    int fd[2];
    int ret=pipe(fd);
    if(ret==-1){
        perror("pipe");
        exit(1);
    }

    pid_t pid=fork();
    if(pid>0){
        close(fd[1]);
        char buf[1024]={0};
        int len=0;

        while((len =read(fd[0],buf,sizeof(buf)-1))>0){
             buf[len] = 0;
            printf("parent receive: %s\n",buf);

        }
        wait(NULL);
    }
    else if(pid==0){
        close(fd[0]);

        dup2(fd[1],STDOUT_FILENO);
        execlp("ps","ps","aux",NULL);
        perror("execlp");//如果运行成功，则会替换子进程中全部内容
        exit(0);
    }
    else{
        perror("fork");
        exit(0);
    }

    return 0;
}