#include "webserver.h"

#include "base/singleton.h"
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
#include <sys/epoll.h>
#include <type_traits>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "db/mysql.h"

namespace hayroc {

WebServer::WebServer(const std::string& ip, int port)
  : m_serverFd(createServerFd(ip, port)),
    m_port(port),
    m_epoll(std::make_shared<Epoll>()),
    m_timerMgr(std::make_shared<TimerMgr>()),
    m_eventLoop(std::make_shared<ThreadPool>()) {
  assert(m_serverFd != -1);

  // init mysql connection pool
  Singleton<MysqlConnectionPool>::GetInstance()->init("localhost", 3306, "root", "root", "MyTinyWebServer");
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
        ERROR("cant accept new conntion");
        break;
      }
    }
    DEBUG("new connect" + std::to_string(connFd));
    std::cout << "new connect: " << connFd << std::endl;
    HttpConnection* httpConn = new HttpConnection(connFd);
    Timer::ptr httpConnTimer = m_timerMgr->addTimer(TIMEOUT, std::bind(&WebServer::closeConnetion, this, httpConn));
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
  DEBUG("关闭连接 end");
}

void WebServer::handleRequest(HttpConnection* httpConnection) {
  DEBUG("处理请求: " + std::to_string(httpConnection->getFd()));

  m_timerMgr->delTimer(httpConnection->getTimer());

  httpConnection->setWorking(true);
  httpConnection->initReq();
  int errorNum;
  int size = httpConnection->read(errorNum);

  if(size == 0 || (size < 0 && errorNum != EAGAIN)) {
    httpConnection->setWorking(false);
    closeConnetion(httpConnection);
  } else {
    httpConnection->setWorking(true);
    Timer::ptr timer = m_timerMgr->addTimer(TIMEOUT, std::bind(&WebServer::closeConnetion, this, httpConnection));
    httpConnection->setTimer(timer);
    httpConnection->initRes();
    httpConnection->process();
    m_epoll->mod(httpConnection->getFd(), (EPOLLOUT | EPOLLET | EPOLLONESHOT), httpConnection);
  }
  DEBUG("处理请求 end");
}

void WebServer::handleResponse(HttpConnection* httpConnection) {
  DEBUG("处理响应");
  m_timerMgr->delTimer(httpConnection->getTimer());

  httpConnection->setWorking(true);
  int errorNum;
  int size = httpConnection->send(errorNum);
  DEBUG("已发送: " + std::to_string(size) + " 待发送: " + std::to_string(httpConnection->toSendSize()) + (size < 0 ? strerror(errorNum) : ""));

  if(httpConnection->toSendSize() == 0) {
    if(!httpConnection->isKeepAlive()) {
      httpConnection->setWorking(false);
      closeConnetion(httpConnection);
    } else { // keep alive add new Timer
      Timer::ptr timer = m_timerMgr->addTimer(TIMEOUT, std::bind(&WebServer::closeConnetion, this, httpConnection));
      httpConnection->setTimer(timer);
      httpConnection->setWorking(false);
      m_epoll->mod(httpConnection->getFd(), (EPOLLIN | EPOLLET | EPOLLONESHOT), httpConnection);
    }
  } else if(size < 0) {
    if(errorNum == EAGAIN) {
      Timer::ptr timer = m_timerMgr->addTimer(TIMEOUT, std::bind(&WebServer::closeConnetion, this, httpConnection));
      httpConnection->setTimer(timer);
      httpConnection->setWorking(true);
      m_epoll->mod(httpConnection->getFd(), (EPOLLOUT | EPOLLET | EPOLLONESHOT), httpConnection);
    } else {
      httpConnection->setWorking(false);
      closeConnetion(httpConnection);
    }
  }
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
  if((serverFd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1) {
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
