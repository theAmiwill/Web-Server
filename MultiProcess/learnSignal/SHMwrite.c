#include<stdio.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>

int main(){
    int shmid = shmget(1234,4096,IPC_CREAT|0664);
    void *ptr=shmat(shmid,NULL,0);
    char*str="hello world";
    memcpy(ptr,str,strlen(str)+1);
    printf("按任意键继续\n");
    getchar();
    
    shmdt(ptr);
    shmctl(shmid,IPC_RMID,NULL);

    return 0;
}