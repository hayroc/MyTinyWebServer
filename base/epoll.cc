#include "epoll.h"

#include "../log/log.h"
#include <memory>

namespace hayroc {
Epoll::Epoll() : m_epollfd(epoll_create1(EPOLL_CLOEXEC)), m_eventList(new epoll_event[MAXCONNECT]) {
}

Epoll::~Epoll() {
  if(m_epollfd) {
    close(m_epollfd);
  }
  delete [] m_eventList;
}

int Epoll::add(int fd, int events, void* ptr) {
  epoll_event event;
  event.events = events;
  event.data.ptr = ptr;
  return epoll_ctl(m_epollfd, EPOLL_CTL_ADD, fd, &event);
}

int Epoll::del(int fd, int events, void* ptr) {
  epoll_event event;
  event.events = events;
  event.data.ptr = ptr;
  return epoll_ctl(m_epollfd, EPOLL_CTL_DEL, fd, &event);
}

int Epoll::mod(int fd, int events, void* ptr) {
  epoll_event event;
  event.events = events;
  event.data.ptr = ptr;
  return epoll_ctl(m_epollfd, EPOLL_CTL_MOD, fd, &event);
}

int Epoll::wait(int timeout) {
  int ret = epoll_wait(m_epollfd, m_eventList, MAXCONNECT, timeout);
  return ret;
}

void Epoll::tick(int serverFd, const ThreadPool::ptr& threadPool, int eventsSum) {
  for(int i = 0; i < eventsSum; ++i) {
    auto e = m_eventList[i];
    // HttpConnection::ptr req = std::make_shared<HttpConnection>(static_cast<HttpConnection*>(e.data.ptr));
    HttpConnection* req = static_cast<HttpConnection*>(e.data.ptr);
    if(req->getFd() == serverFd) {
      threadPool->addJod(m_newConnetCallback);
    } else if(e.events & EPOLLHUP || e.events & EPOLLERR) {
      threadPool->addJod(std::bind(m_closeConnetCallback, req));
    } else if(e.events & EPOLLIN) {
      req->setWorking(true);
      threadPool->addJod(std::bind(m_epollInCallback, req));
    } else if(e.events & EPOLLOUT) {
      req->setWorking(true);
      threadPool->addJod(std::bind(m_epollOutCallback, req));
    } else {
      DEBUG("something eles happen");
    }
  }
}

}
