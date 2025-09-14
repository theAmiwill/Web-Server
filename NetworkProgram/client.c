#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    perror("socket");
    exit(-1);
  }

  struct sockaddr_in serveraddr;
  serveraddr.sin_family = AF_INET;
  inet_pton(AF_INET, "172.17.0.1", &serveraddr.sin_addr.s_addr);
  serveraddr.sin_port = htons(9999);
  int ret = connect(fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
  if (ret == -1) {
    perror("connect");
    exit(-1);
  }

  char receBuf[1024] = {0};
  int i = 0;
  while (1) {
    printf("请按任意键继续...\n");
    getchar(); // 阻塞直到读到一个字符（回车也算）

    // char *data = "hello, I am client";
    sprintf(receBuf, "data:%d\n", i++);
    write(fd, receBuf, strlen(receBuf) + 1);

    ssize_t len1 = read(fd, receBuf, sizeof(receBuf));
    if (len1 == -1) {
      perror("read");
      exit(-1);
    } else if (len1 > 0) {
      printf("rece server data:%s\n", receBuf);
    } else if (len1 == 0) {
      printf("server closed...\n");
      break;
    }
    sleep(1);
  }

  close(fd);

  return 0;
}
