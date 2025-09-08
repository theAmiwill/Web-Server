#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void *callback(void *arg) {
  printf("child thread...,%d\n", *(int *)arg);
  return NULL;
}

int main() {
  pthread_t tid;
  int num = 10;
  int ret = pthread_create(&tid, NULL, callback, (void *)&num);
  if (ret != 0) {
    char *err = strerror(ret);
    printf("error:%s\n", err);
  }

  for (int i = 0; i <= 5; i++) {
    printf("%d\n", i);
  }
  sleep(2);

  return 0;
}