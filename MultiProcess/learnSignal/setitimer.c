#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

int main() {
  struct itimerval new_value;
  new_value.it_interval.tv_sec = 2;
  new_value.it_interval.tv_usec = 0;

  new_value.it_value.tv_sec = 3;
  new_value.it_value.tv_usec = 0;

  int ret = setitimer(ITIMER_REAL, &new_value, NULL);
  printf("starting timer\n");
  if (ret == -1) {
    perror("setitimer");
    exit(1);
  }

  getchar();

  return 0;
}