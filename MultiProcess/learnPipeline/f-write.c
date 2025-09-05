#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main() {

  int ret = access("fifo2", F_OK);
  if (ret == -1) {
    printf("fifo2 does not exist, creating it...\n");
    ret = mkfifo("fifo2", 0664);

    if (ret == -1) {
      perror("mkfifo");
      exit(0);
    }
  }

  int fd = open("fifo2", O_WRONLY);
  if (fd == -1) {
    perror("open");
    exit(0);
  }
  for (int i = 0; i < 40; i++) {
    char buf[1024] = {0};
    sprintf(buf, "hellohello,%d\n", i);
    printf("writing:%d\n", i);
    write(fd, buf, strlen(buf));
    sleep(1);
  }

  close(fd);

  return 0;
}