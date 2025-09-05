#define _POSIX_C_SOURCE 200809L // 启用 POSIX 扩展
#include <signal.h>
#include <stdio.h>

int main() {
  sigset_t set;
  sigemptyset(&set);
  int ret = sigismember(&set, SIGINT);
  if (ret == 0) {
    printf("SIGINT不阻塞\n");
  } else if (ret == 1) {
    printf("SIGINT阻塞\n");
  }
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGQUIT);
  int ret1 = sigismember(&set, SIGINT);
  if (ret1 == 0) {
    printf("SIGINT不阻塞\n");
  } else if (ret1 == 1) {
    printf("SIGINT阻塞\n");
  }
  int ret2 = sigismember(&set, SIGQUIT);
  if (ret2 == 0) {
    printf("SIGQUIT不阻塞\n");
  } else if (ret2 == 1) {
    printf("SIGQUIT阻塞\n");
  }

  sigdelset(&set, SIGQUIT);
  int ret3 = sigismember(&set, SIGQUIT);
  if (ret3 == 0) {
    printf("SIGQUIT不阻塞\n");
  } else if (ret3 == 1) {
    printf("SIGQUIT阻塞\n");
  }

  return 0;
}