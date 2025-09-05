#define _POSIX_C_SOURCE 200809L // 启用 POSIX 扩展
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
  pid_t pid = fork();
  if (pid == 0) {
    for (int i = 0; i < 5; i++) {
      printf("Child process: %d\n", getpid());
      sleep(1);
    }
  } else if (pid > 0) {
    printf("parent process\n");
    sleep(2);
    printf("Killing child process: %d\n", pid);
    kill(pid, SIGINT);
  }

  return 0;
}