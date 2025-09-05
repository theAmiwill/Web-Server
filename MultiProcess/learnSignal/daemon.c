#define _POSIX_C_SOURCE 200809L // 启用 POSIX 扩展
#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/time.h>
#include<time.h>
#include<stdlib.h>
#include<signal.h>
#include<string.h>

void work(int num){
    (void)num;
    time_t tm=time(NULL);
    struct tm * loc=localtime(&tm);
    //char buf[1024];
    //sprintf(buf,"%d-%d-%d %d:%d:%d\n",loc->tm_year,loc->tm_mon,loc->tm_mday,loc->tm_hour,loc->tm_min,loc->tm_sec);
    //printf("%s\n",buf);
    char * str=asctime(loc);
    int fd=open("time.txt",O_RDWR|O_CREAT|O_APPEND,0664);
    write(fd,str,strlen(str));
    close(fd);
}

int main(){
    pid_t pid=fork();
    if(pid>0){
        exit(0);}
    setsid();
    umask(022);
    chdir("/workspaces/Web-Server/MultiProcess/learnSignal");
    dup2(open("/dev/null",O_RDWR),STDIN_FILENO);
    dup2(open("/dev/null",O_RDWR),STDOUT_FILENO);
    dup2(open("/dev/null",O_RDWR),STDERR_FILENO);

    struct sigaction act;
    act.sa_flags=0;
    act.sa_handler=work;
    sigemptyset(&act.sa_mask);
    sigaction(SIGALRM,&act,NULL);

    struct itimerval val;
    val.it_value.tv_sec=2;
    val.it_value.tv_usec=0;
    val.it_interval.tv_sec=2;
    val.it_interval.tv_usec=0;
    setitimer(ITIMER_REAL,&val,NULL);

    while(1){
        pause();
    }

    return 0;
}