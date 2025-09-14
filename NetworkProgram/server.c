#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  int lfd = socket(AF_INET, SOCK_STREAM, 0);
  if (lfd == -1) {
    perror("socket");
    exit(-1);
  }

  struct sockaddr_in saddr;
  saddr.sin_family = AF_INET;
  // inet_pton(AF_INET, "172.17.0.1", &saddr.sin_addr.s_addr);
  saddr.sin_addr.s_addr = INADDR_ANY; // 0
  saddr.sin_port = htons(9999);
  int ret = bind(lfd, (struct sockaddr *)&saddr, sizeof(saddr));
  if (ret == -1) {
    perror("bind");
    exit(-1);
  }

  ret = listen(lfd, 8);
  if (ret == -1) {
    perror("listen");
    exit(-1);
  }

  struct sockaddr_in clientaddr;
  socklen_t len = sizeof(clientaddr);
  int cfd = accept(lfd, (struct sockaddr *)&clientaddr, &len);
  if (cfd == -1) {
    perror("accept");
    exit(-1);
  }

  char clientIP[16];
  inet_ntop(AF_INET, &clientaddr.sin_addr.s_addr, clientIP, sizeof(clientIP));
  unsigned short clientPort = ntohs(clientaddr.sin_port);
  printf("client ip:%s, port is %d\n", clientIP, clientPort);

  char receBuf[1024] = {0};
  while (1) {

    ssize_t len1 = read(cfd, receBuf, sizeof(receBuf));
    if (len1 == -1) {
      perror("read");
      exit(-1);
    } else if (len1 > 0) {
      printf("rece client data:%s\n", receBuf);
    } else if (len1 == 0) {
      printf("client closed...\n");
      break;
    }

    write(cfd, receBuf, len1);
  }

  close(cfd);
  close(lfd);

  return 0;
}