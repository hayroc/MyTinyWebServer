#pragma once

#include <queue>
#include <mutex>
#include <memory>
#include <chrono>
#include <functional>

namespace hayroc {


using Function = std::function<void()>;
using MS = std::chrono::milliseconds;
using Clock = std::chrono::high_resolution_clock;
using TimePoint = Clock::time_point;

class Timer {
public:
  using ptr = std::shared_ptr<Timer>;

  void setUsed(bool used) { m_used = used; }
  bool isUsed() const { return m_used; }
  void runCallbackFunc() const { m_callbackFunc(); }
  TimePoint getExprieTime() const { return m_exprireTime; }

public:
  Timer(const TimePoint& tp, Function callbackFunc) : m_exprireTime(tp), m_callbackFunc(callbackFunc), m_used(true) {}

private:
  TimePoint m_exprireTime;
  Function m_callbackFunc;
  bool m_used;
};

class TimerCmp {
public:
  bool operator()(const std::shared_ptr<Timer> &lhs, const std::shared_ptr<Timer> &rhs) {
    return lhs->getExprieTime() > rhs->getExprieTime();
  }
};

class TimerMgr {
public:
  using ptr = std::shared_ptr<TimerMgr>;
  void updTime() { m_nowTime = Clock::now(); }
  std::shared_ptr<Timer> addTimer(const int& time, Function callbackFunc);
  void delTimer(std::shared_ptr<Timer> timer);
  void tick();
  int getExprieTime();

public:
  TimerMgr() : m_nowTime(Clock::now()) {}

private:
  std::priority_queue<std::shared_ptr<Timer>, std::vector<std::shared_ptr<Timer>>, TimerCmp> m_queue;
  TimePoint m_nowTime;
  std::mutex m_mutex;
};

}
