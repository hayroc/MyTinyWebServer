#pragma once

#include <memory>
#include<sys/epoll.h>
#include<functional>
#include<vector>
#include<unistd.h>
#include<iostream>

#include "../thread/threadpool.h"
#include "../http/httpconnection.h"

namespace hayroc {

using NewConnetCallback = std::function<void()>;
using CloseConnetCallback = std::function<void(HttpConnection*)>;
using EpollInCallback = std::function<void(HttpConnection*)>;
using EpollOutCallback = std::function<void(HttpConnection*)>;

class Epoll {
public:
  using ptr = std::shared_ptr<Epoll>;
  static const int MAXCONNECT = 10000;
  int add(int fd, int events, void* ptr);
  int del(int fd, int events, void* ptr);
  int mod(int fd, int events, void* ptr);
  int wait(int timeOur);
  void tick(int fd, const ThreadPool::ptr& threadPool, int eventsSum);
  void setNewConnetCallback(const NewConnetCallback& func) { m_newConnetCallback = func; }
  void setCloseConnetCallback(const CloseConnetCallback& func) { m_closeConnetCallback = func; }
  void setEpollInCallback(const EpollInCallback& func) { m_epollInCallback= func; }
  void setEpollOutCallback(const EpollOutCallback& func) { m_epollOutCallback= func; }

public:
  Epoll();
  ~Epoll();

private:
  int m_epollfd;
  epoll_event* m_eventList;
  NewConnetCallback m_newConnetCallback;
  CloseConnetCallback m_closeConnetCallback;
  EpollInCallback m_epollInCallback;
  EpollOutCallback m_epollOutCallback;
};

}

