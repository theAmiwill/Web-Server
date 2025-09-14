#include <arpa/inet.h>
#include <stdio.h>
#include <sys/socket.h>

int main() {
  char buf[] = "192.168.1.4";
  unsigned int num = 0;
  inet_pton(AF_INET, buf, &num);
  unsigned char *p = (unsigned char *)&num;
  printf("%d %d %d %d\n", *p, *(p + 1), *(p + 2), *(p + 3));

  char ip[16] = "";
  const char *str = inet_ntop(AF_INET, &num, ip, sizeof(ip));
  printf("str:%s\n", str);
  printf("ip:%s\n", ip);

  return 0;
}