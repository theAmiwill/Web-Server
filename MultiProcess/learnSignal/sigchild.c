#define _POSIX_C_SOURCE 200809L // 启用 POSIX 扩展
#include <signal.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wait.h>

void myfun(int num) {
  printf("capture signal:%d\n", num);
  while (1) {
    int ret = waitpid(-1, NULL, WNOHANG);
    if (ret > 0) {
      printf("child die pid=%d\n", ret);
    } else if (ret == 0) {
      break;
    } else if (ret == -1) {
      break;
    }
  }
}

int main() {

  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGCHLD);
  sigprocmask(SIG_BLOCK, &set, NULL);

  pid_t pid;
  for (int i = 0; i < 20; i++) {
    pid = fork();
    if (pid == 0) {
      break;
    }
  }

  if (pid > 0) {

    struct sigaction act;
    act.sa_flags = 0;
    act.sa_handler = myfun;
    sigemptyset(&act.sa_mask);
    sigaction(SIGCHLD, &act, NULL);

    sigprocmask(SIG_UNBLOCK, &set, NULL);

    while (1) {
      printf("parent process pid:%d\n", getpid());
      sleep(2);
    }
  } else if (pid == 0) {
    printf("child process pid:%d\n", getpid());
  }

  return 0;
}