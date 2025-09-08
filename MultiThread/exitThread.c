#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void *callback(void *arg) {
  printf("child thread...,%d,child id is:%ld\n", *(int *)arg, pthread_self());
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

  for (int i = 0; i <= 500; i++) {
    printf("%d\n", i);
  }

  printf("child thread id:%ld,parent id is:%ld\n", tid, pthread_self());

  pthread_exit(NULL);

  return 0;
}