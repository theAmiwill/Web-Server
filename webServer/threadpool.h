#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "locker.h"
#include <cstdio>
#include <exception>
#include <list>
#include <pthread.h>

template <typename T> class threadpool {
public:
  threadpool(int thread_number = 8, int max_requests = 10000);
  ~threadpool();

  bool append(T *request);

private:
  static void *worker(void *arg);
  void run();

private:
  int m_thread_number;
  pthread_t *m_threads;
  int m_max_requests;
  std::list<T *> m_workqueue;
  locker m_queuelocker;
  sem m_queuestat;
  bool m_stop;
};

template <typename T>
threadpool<T>::threadpool(int thread_number, int max_requests)
    : m_thread_number(thread_number), m_max_requests(max_requests),
      m_stop(false), m_threads(NULL) {
  if ((thread_number <= 0) || (max_requests <= 0)) {
    throw std::exception();
  }

  m_threads = new pthread_t[m_thread_number];
  if (!m_threads) {
    throw std::exception();
  }

  for (int i = 0; i < thread_number; ++i) {
    printf("create the %dth thread\n", i);

    if (pthread_create(m_threads + i, NULL, worker, this) != 0) {
      delete[] m_threads;
      throw std::exception();
    }

    if (pthread_detach(m_threads[i])) {
      delete[] m_threads;
      throw std::exception();
    }
  }
}

template <typename T> threadpool<T>::~threadpool() {
  delete[] m_threads;
  m_stop = true;
}

template <typename T> bool threadpool<T>::append(T *request) {
  m_queuelocker.lock();
  if (m_workqueue.size() > m_max_requests) {
    m_queuelocker.unlock();
    return false;
  }

  m_workqueue.push_back(request);
  m_queuelocker.unlock();
  m_queuestat.post();
  return true;
}

template <typename T> void *threadpool<T>::worker(void *arg) {
  threadpool *pool = (threadpool *)arg;
  pool->run();
  return pool;
}

template <typename T> void threadpool<T>::run() {
  while (!m_stop) {
    m_queuestat.wait();
    m_queuelocker.lock();
    if (m_workqueue.empty()) {
      m_queuelocker.unlock();
      continue;
    }

    T *request = m_workqueue.front();
    m_workqueue.pop_front();
    m_queuelocker.unlock();

    if (!request) {
      continue;
    }

    request->process();
  }
}

#endif

/*
一个请求到来，主线程调用 append。
主线程先拿到互斥锁钥匙 (lock)。
然后把请求放入队列 m_workqueue。
然后归还互斥锁钥匙 (unlock)。
最后按响信号量铃铛 (post)，将订单计数加1，并可能唤醒一个正在睡觉的厨师。
一个空闲的厨师线程之前正因 wait() 而在信号量上睡觉。
铃铛响了，厨师被唤醒，wait() 函数返回。
厨师醒来后，尝试去拿互斥锁钥匙 (lock)。
拿到钥匙后，从 m_workqueue 中取出请求。
归还钥匙 (unlock)。
开始执行 request->process()，处理请求。处理完后，回到 while 循环的开头，继续在
wait() 处睡觉，等待下一个铃铛。
*/