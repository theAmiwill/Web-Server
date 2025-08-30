#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdio.h>

int main(){
    struct stat statbuf;
    int ret=stat("cpy.txt",&statbuf);
    if(ret==-1){
        perror("stat报错");
        return -1;
    }
    printf("File size: %ld bytes\n",statbuf.st_size);
    return 0;
}