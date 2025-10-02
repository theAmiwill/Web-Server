#include "http_conn.h"
#include "locker.h"
#include "lst_timer.h"
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
#define TIMESLOT 5 // 最小超时单位

// --- 从 nonactive_conn.cpp 移植过来的代码 ---
static int pipefd[2];
static sort_timer_lst timer_lst;
static int epollfd = 0;

void sig_handler(int sig) {
  int save_errno = errno;
  int msg = sig;
  send(pipefd[1], (char *)&msg, 1, 0);
  errno = save_errno;
}

void timer_handler() {
  timer_lst.tick();
  alarm(TIMESLOT);
}

// 定时器回调函数，它删除非活动连接socket上的注册事件，并关闭之。
void cb_func(http_conn *user_data) {
  epoll_ctl(epollfd, EPOLL_CTL_DEL, user_data->m_sockfd, 0);
  assert(user_data);
  close(user_data->m_sockfd);
  printf("close fd %d\n", user_data->m_sockfd);
  // 注意：这里我们不直接调用 user_data->close_conn()，因为那会减少 m_user_count
  // 而在主循环中我们可能还需要这个信息。真正的清理在主循环中完成。
}

void addsig(int sig, void(handler)(int), bool restart = true) {
  struct sigaction sa;
  memset(&sa, '\0', sizeof(sa));
  sa.sa_handler = handler;
  if (restart) {
    sa.sa_flags |= SA_RESTART;
  }
  sigfillset(&sa.sa_mask);
  assert(sigaction(sig, &sa, NULL) != -1);
}

extern void addfd(int epollfd, int fd, bool one_shot);
extern void removefd(int epollfd, int fd);
extern void modfd(int epollfd, int fd, int ev);
extern void setnonblocking(int fd);

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

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);
  bind(listenfd, (struct sockaddr *)&address, sizeof(address));

  // 5代表最大监听数
  listen(listenfd, 5);

  epoll_event events[MAX_EVENT_NUM];
  epollfd = epoll_create(5); // 这里的5没有实际意义
  assert(epollfd != -1);

  // 不要使用 http_conn.cpp 中的 addfd，直接在这里设置 listenfd
  epoll_event event;
  event.data.fd = listenfd;
  event.events = EPOLLIN | EPOLLRDHUP;
  epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &event);
  setnonblocking(listenfd); // 确保 listenfd 也是非阻塞的

  http_conn::m_epollfd = epollfd;

  // --- 创建管道和设置信号处理 ---
  int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
  assert(ret != -1);
  setnonblocking(pipefd[1]); // 写端非阻塞

  // 不要使用 http_conn.cpp 中的 addfd，直接在这里设置 pipefd[0]
  event.data.fd = pipefd[0];
  event.events = EPOLLIN | EPOLLRDHUP; // 监听读事件
  epoll_ctl(epollfd, EPOLL_CTL_ADD, pipefd[0], &event);
  // 注意：pipefd[0] 不需要设置 EPOLLONESHOT

  // 添加信号处理
  addsig(SIGALRM, sig_handler, false);
  addsig(SIGTERM, sig_handler, false);
  bool stop_server = false;

  bool timeout = false;
  alarm(TIMESLOT); // 启动定时

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

        // --- 为新连接创建定时器 ---
        util_timer *timer = new util_timer;
        timer->user_data = &users[connfd];
        timer->cb_func = cb_func;
        time_t cur = time(NULL);
        timer->expire = cur + 3 * TIMESLOT;
        users[connfd].timer = timer;
        timer_lst.add_timer(timer);
        // --- 定时器创建结束 ---

      } else if ((sockfd == pipefd[0]) && (events[i].events & EPOLLIN)) {
        // --- 处理信号 ---
        int sig;
        char signals[1024];
        ret = recv(pipefd[0], signals, sizeof(signals), 0);
        if (ret == -1 || ret == 0) {
          continue;
        } else {
          for (int i = 0; i < ret; ++i) {
            switch (signals[i]) {
            case SIGALRM: {
              timeout = true;
              break;
            }
            case SIGTERM: {
              stop_server = true;
            }
            }
          }
        }
        // --- 信号处理结束 ---

      } else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
        // 服务器端关闭连接，移除对应的定时器
        util_timer *timer = users[sockfd].timer;
        if (timer) {
          timer_lst.del_timer(timer);
        }
        users[sockfd].close_conn();

      } else if (events[i].events & EPOLLIN) {
        util_timer *timer = users[sockfd].timer;
        if (users[sockfd].read()) {
          // --- 如果有数据，则重置定时器 ---
          if (timer) {
            time_t cur = time(NULL);
            timer->expire = cur + 3 * TIMESLOT;
            printf("adjust timer once\n");
            timer_lst.adjust_timer(timer);
          }
          pool->append(users + sockfd);
        } else {
          // 对方关闭连接，移除定时器
          if (timer) {
            timer_lst.del_timer(timer);
          }
          users[sockfd].close_conn();
        }
      } else if (events[i].events & EPOLLOUT) {
        util_timer *timer = users[sockfd].timer;
        if (!users[sockfd].write()) {
          // 发送失败，关闭连接，移除定时器
          if (timer) {
            timer_lst.del_timer(timer);
          }
          users[sockfd].close_conn();
        }
      }
    }
    // 最后处理定时事件
    if (timeout) {
      timer_handler();
      timeout = false;
    }
  }

  close(epollfd);
  close(listenfd);
  close(pipefd[1]);
  close(pipefd[0]);
  delete[] users;
  delete pool;

  return 0;
}
