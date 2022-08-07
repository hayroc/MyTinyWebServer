#include "httpconnection.h"

namespace hayroc {

HttpConnection::HttpConnection(int connfd) 
  : m_connFd(connfd),
    m_timer(nullptr),
    m_req(new HttpRequest),
    m_res(new HttpResponse) {
  m_keepAlive = true;
}


HttpConnection::~HttpConnection() {
  if(m_connFd) {
    close(m_connFd);
  }
}

void HttpConnection::process() {
  // parse request
  HttpCode httpCode = m_req->parse();
  DEBUG("after parse the code is " + std::to_string(static_cast<int>(httpCode)));
  // make response
  m_res->init();
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
