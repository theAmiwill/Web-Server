#include<sys/types.h>
#include<sys/wait.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>

int main(){
    pid_t pid;

    for(int i=0;i<5;i++){
        pid=fork();
        if(pid==0){
            break;
        }
    }
    if(pid>0){
        while(1){
            printf("I am parent, pid=%d, ppid=%d\n",getpid(),getppid());

            int status;
            int ret=wait(&status);
            if(ret==-1){
                break;
            }
            printf("child die,pid=%d\n",ret);
            if(WIFEXITED(status)){
                //是否正常退出
                printf("child exit code=%d\n",WEXITSTATUS(status));
            }
            if(WIFSIGNALED(status)){
                //是否异常终止
                printf("child killed by signal=%d\n",WTERMSIG(status));
            }
            sleep(1);
        }
    }
    
    else if(pid==0){
        while(1){
            printf("I am child, pid=%d, ppid=%d\n",getpid(),getppid());
            sleep(1);
        }
        exit(0);//执行到此时子进程结束退出 
    }
    return 0;
}