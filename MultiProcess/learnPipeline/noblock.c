#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int main() {

  int pipefd[2];
  int ret = pipe(pipefd);
  if (ret == -1) {
    perror("pipe");
    exit(1);
  }

  pid_t pid = fork();
  if (pid > 0) {
    printf("i am parent process,pid:%d\n", getpid());
    close(pipefd[1]);
    char buf[1024] = {0};

    int flags = fcntl(pipefd[0], F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(pipefd[0], F_SETFL, flags);

    while (1) {
      int len = read(pipefd[0], buf, sizeof(buf) - 1);
      if (len > 0) {
        buf[len] = 0;
        printf("len:%d\n", len);
        printf("Parent process received: %s,pid:%d\n", buf, getpid());
      } else if (len == 0) {
        // read 返回 0 表示写端已关闭，循环应该结束
        printf("Pipe has been closed. Exiting.\n");
        break;
      } else { // len == -1
        // 在非阻塞模式下，len 为 -1 且 errno 为 EAGAIN 是正常情况
        // 表示管道中暂时没有数据
        printf("No data in pipe (len: %d)\n", len);
      }
      sleep(1);
    }
  } else if (pid == 0) {
    printf("i am child process,pid:%d\n", getpid());
    close(pipefd[0]);
    // char buf1[1024]={0};
    while (1) {
      char *str = "hellohello";
      write(pipefd[1], str, strlen(str));
      sleep(4);
    }
  }
  return 0;
}