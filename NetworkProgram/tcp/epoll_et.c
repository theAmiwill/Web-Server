#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  int lfd = socket(PF_INET, SOCK_STREAM, 0);
  struct sockaddr_in saddr;
  saddr.sin_port = htons(9999);
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = INADDR_ANY;

  bind(lfd, (struct sockaddr *)&saddr, sizeof(saddr));

  listen(lfd, 8);

  int epfd = epoll_create(100);

  struct epoll_event epev;
  epev.events = EPOLLIN;
  epev.data.fd = lfd;
  epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &epev);

  struct epoll_event epevs[1024];

  while (1) {
    int ret = epoll_wait(epfd, epevs, 1024, -1);
    if (ret == -1) {
      perror("epoll_wait");
      exit(-1);
    }
    printf("ret = % d\n", ret);

    for (int i = 0; i < ret; i++) {
      int curfd = epevs[i].data.fd;

      if (curfd == lfd) {
        struct sockaddr_in cliaddr;
        int len = sizeof(cliaddr);
        int cfd = accept(lfd, (struct sockaddr *)&cliaddr, &len);

        int flag = fcntl(cfd, F_GETFL);
        flag = flag | O_NONBLOCK;
        fcntl(cfd, F_SETFL, flag);

        // epev.events = EPOLLIN;
        epev.events = EPOLLIN | EPOLLET;
        epev.data.fd = cfd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &epev);
      } else {
        char buf[5];
        int len = 0;
        while ((len = read(curfd, buf, sizeof(buf))) > 0) {
          // printf("recv data:%s\n", buf);
          write(STDOUT_FILENO, buf, len);
          write(curfd, buf, len);
        }
        if (len == 0) {
          printf("client closed...\n");
        } else if (len == -1) {
          if (errno == EAGAIN) {
            printf("data over...\n");
          } else {
            perror("read\n");
            exit(-1);
          }
        }
      }
    }
  }
  close(lfd);
  close(epfd);

  return 0;
}