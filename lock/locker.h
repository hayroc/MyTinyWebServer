#pragma once

#include <exception>
#include <pthread.h>
#include <semaphore.h>

// 信号量封装
class sem {
public:
  sem(int num = 0) {
    if (sem_init(&m_sem, 0, num) != 0) {
      throw std::exception();
    }
  }
  bool wait() {
    return sem_wait(&m_sem) == 0;
  }
  bool post() {
    return sem_post(&m_sem) == 0;
  }
  ~sem() {
    sem_destroy(&m_sem);
  }

private:
  sem_t m_sem;
};

// 互斥量封装
class locker {
public:
  locker() {
    if (pthread_mutex_init(&m_mutex, nullptr) == 0) {
      throw std::exception();
    }
  }
  bool lock() {
    return pthread_mutex_lock(&m_mutex) == 0;
  }
  bool unlock() {
    return pthread_mutex_unlock(&m_mutex) == 0;
  }
  pthread_mutex_t* get() {
    return &m_mutex;
  }
  ~locker() {
    pthread_mutex_destroy(&m_mutex);
  }

private:
  pthread_mutex_t m_mutex;
};

// 条件变量封装
class cond {
public:
  cond() {
    if (pthread_cond_init(&m_cond, nullptr) != 0) {
      throw std::exception();
    }
  }
  bool wait(pthread_mutex_t* mutex) {
    return pthread_cond_wait(&m_cond, mutex) == 0;
  }
  bool wait(pthread_mutex_t* mutex, timespec* t) {
    return pthread_cond_timedwait(&m_cond, mutex, t) == 0;
  }
  bool signal() {
    return pthread_cond_signal(&m_cond) == 0;
  }
  bool broadcast() {
    return pthread_cond_broadcast(&m_cond) == 0;
  }
  ~cond() {
    pthread_cond_destroy(&m_cond);
  }

private:
  pthread_cond_t m_cond;
};
