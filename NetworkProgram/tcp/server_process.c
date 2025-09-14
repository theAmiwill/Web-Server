#define _POSIX_C_SOURCE 200809L // 启用 POSIX 扩展
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <wait.h>

void recycleChild(int arg) {
  while (1) {
    int ret = waitpid(-1, NULL, WNOHANG);
    if (ret == -1) {
      break;
    } else if (ret == 0) {
      break;
    } else if (ret > 0) {
      printf("子进程%d被回收了\n", ret);
    }
  }
}

int main() {

  struct sigaction act;
  act.sa_flags = 0;
  sigemptyset(&act.sa_mask);
  act.sa_handler = recycleChild;
  sigaction(SIGCHLD, &act, NULL);

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

    pid_t pid = fork();
    if (pid == 0) {
      char clientIP[16];
      inet_ntop(AF_INET, &clientaddr.sin_addr.s_addr, clientIP,
                sizeof(clientIP));
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
      exit(0);
    }
  }
  close(lfd);

  return 0;
}