#define _POSIX_C_SOURCE 200809L // 启用 POSIX 扩展
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

struct sockInfo {
  int fd;
  pthread_t tid;
  struct sockaddr_in addr;
};

struct sockInfo sockInfos[64];

void *working(void *arg) {

  struct sockInfo *tinfo = (struct sockInfo *)arg;
  char clientIP[16];
  inet_ntop(AF_INET, &tinfo->addr.sin_addr.s_addr, clientIP, sizeof(clientIP));
  unsigned short clientPort = ntohs(tinfo->addr.sin_port);
  printf("client ip:%s, port is %d\n", clientIP, clientPort);

  char receBuf[1024] = {0};
  while (1) {

    ssize_t len1 = read(tinfo->fd, receBuf, sizeof(receBuf));
    if (len1 == -1) {
      perror("read");
      exit(-1);
    } else if (len1 > 0) {
      printf("rece client data:%s\n", receBuf);
    } else if (len1 == 0) {
      printf("client closed...\n");
      break;
    }

    write(tinfo->fd, receBuf, len1);
  }
  close(tinfo->fd);
  return NULL;
}

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

  ret = listen(lfd, 128);
  if (ret == -1) {
    perror("listen");
    exit(-1);
  }

  int max = sizeof(sockInfos) / sizeof(sockInfos[0]);
  for (int i = 0; i < max; i++) {
    memset(&sockInfos[i], 0, sizeof(sockInfos[i]));
    sockInfos[i].fd = -1;
    sockInfos[i].tid = -1;
  }

  while (1) {

    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);
    int cfd = accept(lfd, (struct sockaddr *)&clientaddr, &len);
    if (cfd == -1) {
      if (errno == EINTR) {
        continue;
      }
      perror("accept");
      exit(-1);
    }

    struct sockInfo *pinfo;
    for (int i = 0; i < max; i++) {
      if (sockInfos[i].fd == -1) {
        pinfo = &sockInfos[i];
        break;
      }
      if (i == max - 1) {
        sleep(1);
        i--;
      }
    }

    pinfo->fd = cfd;
    memcpy(&pinfo->addr, &clientaddr, len);

    pthread_create(&pinfo->tid, NULL, working, pinfo);

    pthread_detach(pinfo->tid);
  }
  close(lfd);

  return 0;
}