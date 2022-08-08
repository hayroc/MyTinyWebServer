#include "timer.h"
#include <bits/chrono.h>
#include <chrono>
#include <memory>
#include <mutex>

namespace hayroc {

std::shared_ptr<Timer> TimerMgr::addTimer(int time, Function callbackFunc) {
  std::shared_ptr<Timer> timer = std::make_shared<Timer>(Clock::now() + static_cast<MS>(time), callbackFunc);

  if(true) {
    std::unique_lock<std::mutex> lock(m_mutex);
    updTime();
    m_queue.push(timer);
  }
  return timer;
}

void TimerMgr::delTimer(std::shared_ptr<Timer> timer) {
  if(timer != nullptr) {
    timer->setUsed(false);
  }
}

void TimerMgr::tick() {
  std::unique_lock<std::mutex> lock(m_mutex);
  updTime();

  while(!m_queue.empty()) {
    auto timer = m_queue.top();
    if(!timer->isUsed()) {
      m_queue.pop();
      continue;
    }
    if(std::chrono::duration_cast<MS>(timer->getExprieTime() - m_nowTime).count() > 0) {
      break;
    }
    m_queue.pop();
    timer->runCallbackFunc();
  }
}

int TimerMgr::getExprieTime() {
  int exprieTime = 0;
  std::unique_lock<std::mutex> lock(m_mutex);
  while(!m_queue.empty() && !m_queue.top()->isUsed()) {
    m_queue.pop();
  }
  if(m_queue.empty()) {
    return -1; // why not -1
  }

  exprieTime = std::chrono::duration_cast<MS>(m_queue.top()->getExprieTime() - Clock::now()).count();
  return exprieTime < 0 ? 0 : exprieTime;
}

}

