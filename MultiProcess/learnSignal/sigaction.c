#define _POSIX_C_SOURCE 200809L // 启用 POSIX 扩展
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

void myalarm(int num) { printf("captured signal %d\n", num); }

int main() {

  struct sigaction act;
  act.sa_flags = SA_RESTART;
  act.sa_handler = myalarm;
  sigemptyset(&act.sa_mask);

  sigaction(SIGALRM, &act, NULL);

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