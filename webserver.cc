#include "webserver.h"

#include "http/http.h"
#include "log/log.h"
#include "base/epoll.h"
#include "http/httpconnection.h"
#include "thread/threadpool.h"
#include "timer/timer.h"
#include <asm-generic/errno-base.h>
#include <cassert>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <memory>
#include <stdlib.h>
#include <string>
#include <type_traits>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

namespace hayroc {

WebServer::WebServer(const std::string& ip, int port)
  : m_serverFd(createServerFd(ip, port)),
    m_port(port),
    m_epoll(std::make_shared<Epoll>()),
    m_timerMgr(std::make_shared<TimerMgr>()),
    m_eventLoop(std::make_shared<ThreadPool>()) {
  assert(m_serverFd != -1);
  std::cout << "listen on " << ip << ":" << std::to_string(port) << std::endl;
}

WebServer::~WebServer() {
  if(m_serverFd) {
    close(m_serverFd);
  }
}

void WebServer::run() {
  HttpConnection* server = new HttpConnection(m_serverFd);
  m_epoll->add(m_serverFd, (EPOLLIN | EPOLLET), server);
  m_epoll->setNewConnetCallback(std::bind(&WebServer::newConnetion, this));
  m_epoll->setCloseConnetCallback(std::bind(&WebServer::closeConnetion, this, std::placeholders::_1));
  m_epoll->setEpollInCallback(std::bind(&WebServer::handleRequest, this, std::placeholders::_1));
  m_epoll->setEpollOutCallback(std::bind(&WebServer::handleResponse, this, std::placeholders::_1));

  while(true) {
    int time = m_timerMgr->getExprieTime();
    int eventSum = m_epoll->wait(time);
    if(eventSum > 0) {
      m_epoll->tick(m_serverFd, m_eventLoop, eventSum);
    }
    m_timerMgr->tick();
  }
}

void WebServer::newConnetion() { // ET
  DEBUG("新连接");
  while(true) {
    int connFd = ::accept4(m_serverFd, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if(connFd == -1) {
      if(errno == EAGAIN) {
        break;
      } else {
        DEBUG("accept error");
        break;
      }
    }
    DEBUG("新连接" + std::to_string(connFd));
    HttpConnection* httpConn = new HttpConnection(connFd);
    Timer::ptr httpConnTimer = m_timerMgr->addTimer(500, std::bind(&WebServer::closeConnetion, this, httpConn));
    httpConn->setTimer(httpConnTimer);
    m_epoll->add(connFd, (EPOLLIN | EPOLLET | EPOLLONESHOT), httpConn);
  }
  DEBUG("新连接 end");
}

void WebServer::closeConnetion(HttpConnection* httpConnection) {
  DEBUG("关闭连接");
  if(httpConnection->isWorking()) {
    return ;
  }

  int fd = httpConnection->getFd();
  m_timerMgr->delTimer(httpConnection->getTimer());
  m_epoll->del(fd, 0, httpConnection);
  delete httpConnection;
  DEBUG("新连接 end");
}

void WebServer::handleRequest(HttpConnection* httpConnection) {
  DEBUG("处理请求");

  m_timerMgr->delTimer(httpConnection->getTimer());
  httpConnection->setWorking(true);

  int errorNum;
  int size = httpConnection->read(errorNum);
  DEBUG("处理请求 1");

  if(size == 0 || (size < 0 && errorNum != EAGAIN)) {
    DEBUG("处理请求 2");
    httpConnection->setWorking(false);
    closeConnetion(httpConnection);
  } else {
    httpConnection->process();
    m_epoll->mod(httpConnection->getFd(), (EPOLLIN | EPOLLOUT | EPOLLET | EPOLLONESHOT), httpConnection);
  }
  DEBUG("处理请求 end");
}

void WebServer::handleResponse(HttpConnection* httpConnection) {
  DEBUG("处理响应");
  m_timerMgr->delTimer(httpConnection->getTimer());
  httpConnection->setWorking(true);

  int errorNum;
  int size = httpConnection->send(errorNum);

  if(httpConnection->toSendSize() == 0) {
    if(!httpConnection->isKeepAlive()) {
      std::cout << "i am here" << "\n";
      closeConnetion(httpConnection);
    }
  } else if(size < 0) {
    std::cout << httpConnection->getFd() <<  " to Send Size: " << httpConnection->toSendSize() << std::endl;
    if(errorNum == EAGAIN) {
      Timer::ptr timer = m_timerMgr->addTimer(500, std::bind(&WebServer::closeConnetion, this, httpConnection));
      httpConnection->setTimer(timer);
      httpConnection->setWorking(true);
      m_epoll->mod(httpConnection->getFd(), (EPOLLIN | EPOLLOUT | EPOLLET | EPOLLONESHOT), httpConnection);
    } else {
      closeConnetion(httpConnection);
    }
  }

  /*
  if(size == -1 && errorNum != EAGAIN) {
    httpConnection->setWorking(false);
    closeConnetion(httpConnection);
  } else {
    if(httpConnection->toSendSize() == 0 && !httpConnection->isKeepAlive()) {
      closeConnetion(httpConnection);
    } else {
      Timer::ptr timer = m_timerMgr->addTimer(500, std::bind(&WebServer::closeConnetion, this, httpConnection));
      httpConnection->setTimer(timer);
      m_epoll->mod(httpConnection->getFd(), (EPOLLIN | EPOLLOUT | EPOLLET | EPOLLONESHOT), httpConnection);
      if(errorNum == EAGAIN) {
        httpConnection->setWorking(true);
      } else {
        httpConnection->setWorking(false);
      }
    }
  }
  */
  DEBUG("处理响应 end");
}

int WebServer::createServerFd(const std::string& ip, int port) {
  assert(port >= 1024 && port <= 65535);

  struct sockaddr_in serverAddr;  //设置地址
  bzero(&serverAddr.sin_zero, sizeof(serverAddr.sin_zero));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = inet_addr(ip.data());
  serverAddr.sin_port = htons(port);

  int serverFd;
  int optval = 1;
  if((serverFd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1){
    close(serverFd);
    return -1;
  }
  if(setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int)) == -1) {
    close(serverFd);
    return -1;
  }
  if(bind(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
    close(serverFd);
    return -1;
  }
  if(listen(serverFd, MAXCONNECT) == -1) {
    close(serverFd);
    return -1;
  }
  return serverFd;
}

bool WebServer::setNoBlocking(int fd) {
  int old_option = fcntl(fd, F_GETFL);
  int new_option = old_option | O_NONBLOCK;
  return fcntl(fd, F_SETFL, new_option) != -1;
}

}
