#include "threadpool.h"
#include <mutex>
#include <iostream>

namespace hayroc {

void ThreadPool::addJod(const Function& job) {
  if(true) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_jobs.push(job);
  }
  m_cond.notify_one();
}

ThreadPool::ThreadPool(int maxWorker) {
  assert(maxWorker >= 1);
  for(int i = 0; i < maxWorker; ++i) {
    m_worker.emplace_back([this]() {
      while(!m_stop) {
        Function job;
        if(true) {
          std::unique_lock<std::mutex> lock(m_mutex);
          while(m_jobs.empty()) {
            m_cond.wait(lock);
          }
          if(m_stop) return ;

          job = m_jobs.front();
          m_jobs.pop();
        }
        job();
      }
    });
  }
}

ThreadPool::~ThreadPool() {
  std::cout << "线程池析构" << std::endl;
  if(true) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_stop = true;
  }
  m_cond.notify_all();
  for(auto& t : m_worker) {
    t.join();
  }
}

}
