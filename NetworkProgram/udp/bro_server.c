#include <netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <sys/socket.h>
#include<unistd.h>
#include<arpa/inet.h>

int main(){
    int fd=socket(PF_INET,SOCK_DGRAM,0);
    if(fd==-1){
        perror("socket");
        exit(-1);
    }

    int op=1;
    setsockopt(fd,SOL_SOCKET,SO_BROADCAST,&op,sizeof(op));

    struct sockaddr_in cliaddr;
    cliaddr.sin_family=AF_INET;
    cliaddr.sin_port=htons(9999);
    inet_pton(AF_INET,"172.17.255.255",&cliaddr.sin_addr.s_addr);

    int num=0;
    while(1){
        char sendbuf[128]={0};
        sprintf(sendbuf,"hello,client%d\n",num++);

        sendto(fd,sendbuf,strlen(sendbuf)+1,0,(struct sockaddr*)&cliaddr,sizeof(cliaddr));
        printf("broadcast data:%s\n",sendbuf);
        sleep(1);
    }

    close(fd);
    return 0;
}