#include "http_conn.h"
#include "locker.h"
#include "threadpool.h"
#include <arpa/inet.h>
#include <cassert>
#include <cerrno>
#include <csignal>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#define MAX_FD 65535
#define MAX_EVENT_NUM 10000

void addsig(int sig, void(handler)(int)) {
  struct sigaction sa;
  memset(&sa, '\0', sizeof(sa));
  sa.sa_handler = handler;
  sigfillset(&sa.sa_mask);
  assert(sigaction(sig, &sa, NULL) != -1);
}

extern void addfd(int epollfd, int fd, bool one_shot);
extern void removefd(int epollfd, int fd);
extern void modfd(int epollfd, int fd, int ev);

int main(int argc, char *argv[]) {
  if (argc <= 1) {
    printf("use this format: %s port_number\n", basename(argv[0]));
    exit(-1);
  }

  int port = atoi(argv[1]);

  addsig(SIGPIPE, SIG_IGN);

  threadpool<http_conn> *pool = NULL;
  try {
    pool = new threadpool<http_conn>;
  } catch (...) {
    exit(-1);
  }

  http_conn *users = new http_conn[MAX_FD];

  int listenfd = socket(PF_INET, SOCK_STREAM, 0);

  int reuse = 1;
  setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  int ret;
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);
  ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));

  // 5代表最大监听数
  ret = listen(listenfd, 5);

  epoll_event events[MAX_EVENT_NUM];
  int epollfd = epoll_create(5); // 这里的5没有实际意义

  addfd(epollfd, listenfd, false); // 添加监听的文件描述符
  http_conn::m_epollfd = epollfd;

  while (true) {
    int num = epoll_wait(epollfd, events, MAX_EVENT_NUM, -1);
    if ((num < 0) && (errno != EINTR)) {
      printf("epoll failure\n");
      break;
    }

    for (int i = 0; i < num; i++) {
      int sockfd = events[i].data.fd;
      if (sockfd == listenfd) { // 代表有客户端连接进来
        struct sockaddr_in client_address;
        socklen_t client_addrlen = sizeof(client_address);

        int connfd = accept(listenfd, (struct sockaddr *)&client_address,
                            &client_addrlen);
        if (connfd < 0) {
          printf("errno is: %d\n", errno);
          continue;
        }
        if (http_conn::m_user_count >= MAX_FD) {
          // TODO 尝试给客户端返回信息: 服务器正忙
          close(connfd);
          continue;
        }
        // 将新的客户端数据初始化，放到数组中
        users[connfd].init(connfd, client_address);
      } else if (events[i].events & (EPOLLRDHUP | EPOLLHUP |
                                     EPOLLERR)) { // 对方异常断开或者错误等事件
        users[sockfd].close_conn();
      } else if (events[i].events & EPOLLIN) { // 是否包含读事件
        if (users[sockfd].read()) {
          pool->append(users + sockfd); // 指针运算，得到users[sockfd]的地址
        } else {
          users[sockfd].close_conn();
        }
      } else if (events[i].events & EPOLLOUT) {
        if (!users[sockfd].write()) {
          users[sockfd].close_conn();
        }
      }
    }
  }

  close(epollfd);
  close(listenfd);
  delete[] users;
  delete pool;

  return 0;
}
