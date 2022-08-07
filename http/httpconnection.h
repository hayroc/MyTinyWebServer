#pragma once

#include <memory>
#include <string>
#include <unistd.h>

#include "../timer/timer.h"
#include "http.h"
#include "httprequest.h"
#include "httpresponse.h"
#include "../log/log.h"

namespace hayroc {

class HttpRequest;
class HttpResponse;

using Map = std::map<std::string, std::string>;


class HttpConnection {
public:
  using ptr = std::shared_ptr<HttpConnection>;
  static const int TIMEOUT = 500;

  int read(int &errorNum) { return m_req->recv(m_connFd, errorNum); }
  int send(int &errorNum) { return m_res->send(m_connFd, errorNum); }
  void process();
  HttpCode parse();
  void makeResponse();

  void setHttpCode(HttpCode code) { m_res->setHttpCode(code); }
  void setTimer(const Timer::ptr timer) { m_timer = timer; }
  void setWorking(bool working) { m_working = working; }
  bool isWorking() const { return m_working; }
  bool isKeepAlive() const { return m_keepAlive; }
  int getFd() const { return m_connFd; }
  int toSendSize() const { return m_res->toSendSize(); }
  Timer::ptr getTimer() { return m_timer; }
  HttpRequest::ptr req() { return m_req; }
  HttpResponse::ptr res() { return m_res; }

public:
  HttpConnection(int connfd);
  ~HttpConnection();

private:
  bool m_keepAlive;
  bool m_working;
  int m_connFd;
  Timer::ptr m_timer;
  HttpRequest::ptr m_req;
  HttpResponse::ptr m_res;
};

}
