#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
  int ret = access("fifo1", F_OK);
  if (ret == -1) {
    printf("fifo1 does not exist,creating...\n");
    ret = mkfifo("fifo1", 0664);
    if (ret == -1) {
      perror("mkfifo");
      exit(0);
    }
  }

  int ret1 = access("fifo0", F_OK);
  if (ret1 == -1) {
    printf("fifo0 does not exist,creating...\n");
    ret1 = mkfifo("fifo0", 0664);
    if (ret1 == -1) {
      perror("mkfifo");
      exit(0);
    }
  }

  int fdw = open("fifo1", O_WRONLY);
  if (fdw == -1) {
    perror("open");
    exit(0);
  }
  printf("open fifo1 success,fdw = %d,waiting write\n", fdw);

  int fdr = open("fifo0", O_RDONLY);
  if (fdr == -1) {
    perror("open");
    exit(0);
  }
  printf("open fifo0 success,fdr = %d,waiting read\n", fdr);


  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    exit(0);
  }
  else if(pid>0) {
    char buf[128];
    while (1) {
      memset(buf, 0, 128);
      fgets(buf, 128, stdin);
      ret = write(fdw, buf, strlen(buf));
      if (ret == -1) {
        perror("write");
        exit(0);
        }
    }
  }
  else if(pid==0) {
    char buf[128];
    while (1) {
      memset(buf, 0, 128);
      ret = read(fdr, buf, 128);
      if (ret <= 0) {
        perror("read");
        exit(0);
      }
      printf("chatB say:%s\n", buf);
    }
  }

  close(fdw);
  close(fdr);

  return 0;
}
