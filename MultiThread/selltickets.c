#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

int tickets = 100;

pthread_mutex_t mutex;

void *sellticket(void *arg) {

  while (1) {

    pthread_mutex_lock(&mutex);

    if (tickets > 0) {
      struct timespec ts = {.tv_sec = 0, .tv_nsec = 6000*1000}; // 3000us
      nanosleep(&ts, NULL);
      printf("%ld 正在卖第%d 张门票\n  ", pthread_self(), tickets);
      tickets--;
    } else {
      pthread_mutex_unlock(&mutex);
      break;
    }
    pthread_mutex_unlock(&mutex);
  }

  return NULL;
}

int main() {

  pthread_mutex_init(&mutex, NULL);

  pthread_t tid1, tid2, tid3;
  pthread_create(&tid1, NULL, sellticket, NULL);
  pthread_create(&tid2, NULL, sellticket, NULL);
  pthread_create(&tid3, NULL, sellticket, NULL);

  pthread_join(tid1, NULL);
  pthread_join(tid2, NULL);
  pthread_join(tid3, NULL);

  pthread_mutex_destroy(&mutex);

  return 0;
}