#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void *callback(void *arg) {
  printf("child thread id:%ld\n", pthread_self());
  return NULL;
}

int main() {
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  pthread_t tid;
  int ret = pthread_create(&tid, &attr, callback, NULL);
  if (ret != 0) {
    char *errstr = strerror(ret);
    printf("Thread creation failed: %s\n", errstr);
  }

  size_t size;
  pthread_attr_getstacksize(&attr, &size);
  printf("thread stack size:%ld\n", size);

  printf("main thread id:%ld\n, tid is:%ld\n", pthread_self(), tid);

  pthread_attr_destroy(&attr);

  pthread_exit(NULL);

  return 0;
}