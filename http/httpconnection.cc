#include "httpconnection.h"
#include <iostream>

namespace hayroc {

HttpConnection::HttpConnection(int connfd) 
  : m_working(false),
    m_connFd(connfd),
    m_timer(nullptr),
    m_req(new HttpRequest),
    m_res(new HttpResponse) {
}

HttpConnection::~HttpConnection() {
  std::cout << "close connect: " << m_connFd << std::endl;
  if(m_connFd) {
    close(m_connFd);
  }
}

void HttpConnection::initReq() {
  m_req->init();
}

void HttpConnection::initRes() {
  m_res->init();
}

void HttpConnection::process() {
  // parse request
  HttpCode httpCode = m_req->parse();
  DEBUG("after parse the code is " + std::to_string(static_cast<int>(httpCode)));
  // make response
  m_res->setHttpCode(httpCode);
  m_res->setPath(m_req->getPath());
  m_res->setKeepAlive(m_req->isKeepAlive());
  m_res->setParams(m_req->getParams());
  m_res->makeResponse();
}

HttpCode HttpConnection::parse() {
  return m_req->parse();
}

void HttpConnection::makeResponse() {
  m_res->makeResponse();
}

}
