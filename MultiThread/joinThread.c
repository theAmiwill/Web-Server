#define _POSIX_C_SOURCE 200809L // 启用 POSIX 扩展
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void *callback(void *arg) {
  printf("child thread...,%d,child id is:%ld\n", *(int *)arg, pthread_self());
  sleep(3);
  int *value = malloc(sizeof(int));
  if (value == NULL) {
    perror("malloc failed");
    pthread_exit(NULL);
  }
  *value = 10;
  // pthread_exit((void *)value);
  return (void *)value;
}

int main() {
  pthread_t tid;
  int num = 10;
  int ret = pthread_create(&tid, NULL, callback, (void *)&num);
  if (ret != 0) {
    char *err = strerror(ret);
    printf("error:%s\n", err);
  }

  for (int i = 0; i <= 50; i++) {
    printf("%d\n", i);
  }

  printf("child thread id:%ld,parent id is:%ld\n", tid, pthread_self());

  void *retval;
  ret = pthread_join(tid, &retval);
  if (ret != 0) {
    char *err = strerror(ret);
    printf("error:%s\n", err);
  }

  printf("exit data:%d\n", *(int *)retval);

  printf("子线程资源回收成功\n");

  printf("exit data:%d\n", *(int *)retval);

  free(retval);

  return 0;
}