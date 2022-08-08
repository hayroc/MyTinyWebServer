#pragma once

#include "base/epoll.h"
#include "timer/timer.h"
#include "thread/threadpool.h"
#include "http/httpconnection.h"
#include <string>
#include <cstdio>

namespace hayroc {

class WebServer {
public:
  static const int MAXCONNECT = 10000;
  static const int TIMEOUT = 10000;
  void run();

private:
  void newConnetion();
  void closeConnetion(HttpConnection* httpConnection);
  void handleRequest(HttpConnection* httpConnection);
  void handleResponse(HttpConnection* httpConnection);
  int createServerFd(const std::string& ip, int port);
  bool setNoBlocking(int fd);

public:
  WebServer(const std::string& ip = "127.0.0.1", int port = 3000);
  ~WebServer();

private:
  int m_serverFd;
  int m_port;
  Epoll::ptr m_epoll;
  TimerMgr::ptr m_timerMgr;
  ThreadPool::ptr m_eventLoop;
};

}
