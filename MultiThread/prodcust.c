#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

pthread_mutex_t mutex;
pthread_cond_t cond;

struct Node {
  int num;
  struct Node *next;
};

struct Node *head = NULL;

void *producer(void *arg) {
  while (1) {
    pthread_mutex_lock(&mutex);
    struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
    newNode->next = head;
    head = newNode;
    newNode->num = rand() % 100;
    printf("add node,bum:%d, tid:%ld\n", newNode->num, pthread_self());

    pthread_cond_signal(&cond);

    pthread_mutex_unlock(&mutex);
    struct timespec ts = {.tv_sec = 0, .tv_nsec = 8000 * 1000}; // 3000us
    nanosleep(&ts, NULL);
  }
  return NULL;
}

void *customer(void *arg) {
  struct timespec ts = {.tv_sec = 0, .tv_nsec = 8000 * 1000}; // 6000us
  while (1) {
    pthread_mutex_lock(&mutex);

    while (head == NULL) {
      // wait函数调用时会对互斥锁进行解锁；当不阻塞的时候，继续向下执行，会重新加锁
      pthread_cond_wait(&cond, &mutex);
    }

    struct Node *temp = head;
    head = head->next;
    printf("del node,num:%d,tid；%ld\n", temp->num, pthread_self());
    free(temp);
    pthread_mutex_unlock(&mutex);
    nanosleep(&ts, NULL);
  }
  return NULL;
}

int main() {
  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&cond, NULL);
  pthread_t ptids[5], ctids[5];

  for (int i = 0; i < 5; i++) {
    pthread_create(&ptids[i], NULL, producer, NULL);
    pthread_create(&ctids[i], NULL, customer, NULL);
  }

  for (int i = 0; i < 5; i++) {
    pthread_detach(ptids[i]);
    pthread_detach(ctids[i]);
  }
  // 这里也可以用pthread_join,调用结束前阻塞，就不用睡眠等待了
  while (1) {
    sleep(10);
  }

  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond);

  pthread_exit(NULL);

  return 0;
}