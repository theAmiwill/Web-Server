#include<stdio.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>

int main(){
    int shmid = shmget(1234,0,IPC_CREAT);
    void *ptr=shmat(shmid,NULL,0);
    printf("%s\n",(char*)ptr);
    printf("按任意键继续\n");
    getchar();
    
    shmdt(ptr);
    shmctl(shmid,IPC_RMID,NULL);

    return 0;
}