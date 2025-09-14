#include <arpa/inet.h>
#include <stdio.h>

int main() {
  unsigned short a = 0x0102;
  printf("a:%x\n", a);
  unsigned short b = htons(a);
  printf("b:%x\n", b);

  printf("\n");

  char buf[4] = {192, 168, 1, 100};
  int num = *(int *)buf;
  int num1 = htonl(num);
  unsigned char *p = (char *)&num1;

  printf("%d %d %d %d\n", *p, *(p + 1), *(p + 2), *(p + 3));

  printf("\n");

  char buf1[4] = {190, 160, 1, 10};
  int num2 = *(int *)buf1;
  int num11 = ntohl(num2);
  unsigned char *p1 = (char *)&num11;

  printf("%d %d %d %d\n", *p1, *(p1 + 1), *(p1 + 2), *(p1 + 3));

  printf("\n");

  unsigned short c = 0x0304;
  printf("c:%x\n", c);
  unsigned short d = ntohs(c);
  printf("d:%x\n", d);

  return 0;
}