#include<sys/types.h>
#include<unistd.h>
#include<stdio.h>

int main(){
    pid_t pid=fork();
    if(pid==0){
        execl("/bin/ps","ps","aux",NULL);
        printf("child process\n");
    }
    else if(pid>0){
        printf("Parent process: Child PID is %d\n", pid);
    }
    for(int i=0;i<5;i++){
        printf("process: %d, pid: %d\n", i, getpid());
    }
    return 0;
}
