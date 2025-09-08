#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

int num = 1;
pthread_rwlock_t rwlock;

void *writeNum(void *arg) {
  while (1) {
    pthread_rwlock_wrlock(&rwlock);
    num++;
    printf("++write,tid:%ld,num:%d\n", pthread_self(), num);
    pthread_rwlock_unlock(&rwlock);
    struct timespec ts = {.tv_sec = 0, .tv_nsec = 2000 * 1000}; // 3000us
    nanosleep(&ts, NULL);
  }
  return 0;
}

void *readNum(void *arg) {
  while (1) {
    pthread_rwlock_rdlock(&rwlock);
    printf("==read,tid:%ld,num:%d\n", pthread_self(), num);
    pthread_rwlock_unlock(&rwlock);
    struct timespec ts = {.tv_sec = 0, .tv_nsec = 2000 * 1000}; // 3000us
    nanosleep(&ts, NULL);
  }
  return 0;
}

int main() {
  pthread_rwlock_init(&rwlock, NULL);

  pthread_t wtids[3], rtids[5];
  for (int i = 0; i < 3; i++) {
    pthread_create(&wtids[i], NULL, writeNum, NULL);
  }
  for (int i = 0; i < 5; i++) {
    pthread_create(&rtids[i], NULL, readNum, NULL);
  }

  for (int i = 0; i < 3; i++) {
    pthread_detach(wtids[i]);
  }
  for (int i = 0; i < 5; i++) {
    pthread_detach(rtids[i]);
  }

  pthread_exit(NULL);

  pthread_rwlock_destroy(&rwlock);

  return 0;
}