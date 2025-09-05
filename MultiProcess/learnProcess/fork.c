#include<sys/types.h>
#include<unistd.h>
#include<stdio.h>

int main(){
    printf("begin\n");
    pid_t pid =fork();
    if(pid>0){
        printf("I am parent, pid=%d, ppid=%d\n",getpid(),getppid());
        for(int i=0;i<6;i++){
        printf("i,%d\n",i);
        sleep(1);
    }
    }
    else if(pid==0){
        printf("I am child, pid=%d, ppid=%d\n",getpid(),getppid());
        for(int j=0;j<6;j++){
        printf("j,%d\n",j);
        sleep(1);
    }
    }
    return 0;
}