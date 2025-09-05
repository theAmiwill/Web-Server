
#define _POSIX_C_SOURCE 200809L // 启用 POSIX 扩展
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGQUIT);

  sigprocmask(SIG_BLOCK, &set, NULL);
  int num = 0;

  while (1) {
    num++;
    sigset_t pendingset;
    sigemptyset(&pendingset);
    sigpending(&pendingset);

    for (int i = 1; i <= 31; i++) {
      if (sigismember(&pendingset, i) == 1) {
        printf("1");
      } else if (sigismember(&pendingset, i) == 0) {
        printf("0");
      } else {
        perror("fault");
        exit(0);
      }
    }
    printf("-------\n");
    sleep(1);

    if (num == 20) {
      sigprocmask(SIG_UNBLOCK, &set, NULL);
    }
  }

  return 0;
}
