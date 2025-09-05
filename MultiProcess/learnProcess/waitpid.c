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
            sleep(1);

            int status;
            int ret=waitpid(-1,&status,WNOHANG);
            if(ret==-1){
                break;
            }
            else if(ret==0){
                //还有子进程存在
                continue;
            }
            else if(ret>0){
                if(WIFEXITED(status)){
                    //是否正常退出
                    printf("child exit code=%d\n",WEXITSTATUS(status));
                }
                if(WIFSIGNALED(status)){
                    //是否异常终止
                    printf("child killed by signal=%d\n",WTERMSIG(status));
                }
            }
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