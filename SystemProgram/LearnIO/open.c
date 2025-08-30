#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<stdio.h>
#include<unistd.h>

int main(){
    int fd=open("a.txt",O_RDONLY);
    if(fd==-1)
        perror("出现报错");
    close(fd);
    return 0;
}