#define _POSIX_C_SOURCE 200809L // 打开POSIX扩展，使得truncate函数可用
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

int main() {
  int len = 4096;

  void *ptr = mmap(NULL, len, PROT_READ | PROT_WRITE,
                   MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  if (ptr == MAP_FAILED) {
    perror("mmap");
    exit(0);
  }

  pid_t pid = fork();

  if (pid > 0) {
    strcpy((char *)ptr, "hello,world");
    wait(NULL);
  } else if (pid == 0) {
    sleep(1);
    printf("child read data:%s\n", (char *)ptr);
  }

  int ret = munmap(ptr, len);
  if (ret == -1) {
    perror("munmap");
    exit(0);
  }

  return 0;
}