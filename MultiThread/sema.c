#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

pthread_mutex_t mutex;
sem_t psem;
sem_t csem;

struct Node {
  int num;
  struct Node *next;
};

struct Node *head = NULL;

void *producer(void *arg) {
  struct timespec ts = {.tv_sec = 0, .tv_nsec = 8000 * 1000}; // 3000us
  while (1) {
    sem_wait(&psem);
    pthread_mutex_lock(&mutex);
    struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
    newNode->next = head;
    head = newNode;
    newNode->num = rand() % 100;
    printf("add node,bum:%d, tid:%ld\n", newNode->num, pthread_self());

    pthread_mutex_unlock(&mutex);

    sem_post(&csem);

    nanosleep(&ts, NULL);
  }
  return NULL;
}

void *customer(void *arg) {
  struct timespec ts = {.tv_sec = 0, .tv_nsec = 8000 * 1000}; // 6000us
  while (1) {
    sem_wait(&csem);
    pthread_mutex_lock(&mutex);

    struct Node *temp = head;
    head = head->next;
    printf("del node,num:%d,tid；%ld\n", temp->num, pthread_self());
    free(temp);
    pthread_mutex_unlock(&mutex);

    sem_post(&psem);
    nanosleep(&ts, NULL);
  }
  return NULL;
}

int main() {

  pthread_mutex_init(&mutex, NULL);
  sem_init(&psem, 0, 8);
  sem_init(&csem, 0, 0);
  pthread_t ptids[5], ctids[5];

  for (int i = 0; i < 5; i++) {
    pthread_create(&ptids[i], NULL, producer, NULL);
    pthread_create(&ctids[i], NULL, customer, NULL);
  }

  for (int i = 0; i < 5; i++) {
    pthread_join(ptids[i], NULL);
    pthread_join(ctids[i], NULL);
  }
  // 这里也可以用pthread_join,调用结束前阻塞，就不用睡眠等待了
  // while (1) {
  // sleep(10);
  //}

  sem_destroy(&psem);
  sem_destroy(&csem);
  pthread_mutex_destroy(&mutex);

  pthread_exit(NULL);

  return 0;
}