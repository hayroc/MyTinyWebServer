#pragma once

#include <assert.h>
#include <memory>
#include <queue>
#include <mutex>
#include <vector>
#include <thread>
#include <functional>
#include <condition_variable>

namespace hayroc {

using Function = std::function<void()>;

class ThreadPool {
public:
  using ptr = std::shared_ptr<ThreadPool>;
  void addJod(const Function& job);

public:
  ThreadPool(int maxWorker = 8);
  ~ThreadPool();

private:
  std::vector<std::thread> m_worker;
  std::queue<Function> m_jobs;
  std::mutex m_mutex;
  std::condition_variable m_cond;
  bool m_stop;
};

}
