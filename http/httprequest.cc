#include "httprequest.h"

#include "../log/log.h"
#include "http.h"
#include <string>
#include <utility>
#include <iostream>

namespace hayroc {

HttpRequest::HttpRequest() {
  m_method = Method::GET;
  m_parseStatus = ParseStatus::RequestLine;
}

HttpRequest::~HttpRequest() {

}

int HttpRequest::recv(int fd, int &errorNum) {
  int ret = m_buffer.readFd(fd, errorNum);
  return ret;
}

HttpCode HttpRequest::parse() {
  DEBUG("--- PARSE ---");
  DEBUG(m_buffer.peek());

  while(true) {
    switch(m_parseStatus) {
      case ParseStatus::RequestLine: {
        auto httpCode = parseRequestLine();
        if(httpCode == HttpCode::OK) {
          m_parseStatus = ParseStatus::Header;
        } else {
          DEBUG("RequestLine 请求行错误");
          return httpCode;
        }
        break;
      }
      case ParseStatus::Header: {
        auto httpCode = parseHeader();
        DEBUG("解析请求头： " + std::to_string(static_cast<int>(httpCode)));
        if(httpCode == HttpCode::OK && m_method == Method::GET) {
          httpCode = parsePath();
          return httpCode;
        } else if(httpCode == HttpCode::OK && m_method == Method::POST) {
          m_parseStatus = ParseStatus::Content;
        } else {
          DEBUG("RequestHeader 请求头错误");
          return httpCode;
        }
        break;
      }
      case ParseStatus::Content: {
        auto httpCode = parseContent();
        httpCode = parsePath();
        if(httpCode != HttpCode::OK) {
          return httpCode;
        } else {
          httpCode = parseParams();
          return httpCode;
        }
        break;
      }
      default: {
        DEBUG("defalut internal server error " + std::to_string(static_cast<int>(m_parseStatus)));
        std::cout << static_cast<int>(m_parseStatus) << "\n";
        return HttpCode::InternalServerError;
      }
    }
  }
}

HttpCode HttpRequest::parseRequestLine() {
  const char* SPACE = " ";
  const char* crlf = m_buffer.findCRLF();
  const char* firstSpace = std::search(m_buffer.peek(), crlf, SPACE, SPACE + 1);
  const char* secondSpace = std::search(firstSpace + 1, crlf, SPACE, SPACE + 1);

  if(firstSpace == crlf || secondSpace == crlf) {
    m_method = Method::Invalid;
    DEBUG("解析请求头错误");
    return HttpCode::BadRequest;
  }

  // method
  if(std::equal(m_buffer.peek(), firstSpace - 1, "GET")) {
    m_method = Method::GET;
  } else if(std::equal(m_buffer.peek(), firstSpace - 1, "POST")) {
    m_method = Method::POST;
  } else {
    m_method = Method::Invalid;
    DEBUG("解析请求头错误");
    return HttpCode::MethodNotAllowed;
  }

  // url
  m_url = std::string(firstSpace + 1, secondSpace);

  // version
  if(std::equal(secondSpace + 1, crlf, "HTTP/1.1")) {
    m_version = Version::Http11;
  } else if(std::equal(secondSpace + 1, crlf, "HTTP/1.0")) {
    m_version = Version::Http10;
  } else {
    m_version = Version::Unknow;
  }
  m_buffer.retrieve(crlf - m_buffer.peek() + 2);
  return HttpCode::OK;
}

HttpCode HttpRequest::parseHeader() {
  const char* COLON = ":";
  DEBUG(m_buffer.peek());
  while(true) {
    const char* crlf = m_buffer.findCRLF();
    if(crlf == m_buffer.peek()) {
      m_buffer.retrieve(2);
      break;
    }
    const char* colon = std::search(m_buffer.peek(), crlf, COLON, COLON + 1);
    if(colon != crlf) {
      m_headers.insert(std::make_pair(std::string(m_buffer.peek(), colon), std::string(colon + 2, crlf)));
    } else {
      DEBUG("cannot parse header: " + std::string(m_buffer.peek(), crlf));
      return HttpCode::BadRequest;
    }
    m_buffer.retrieve(crlf - m_buffer.peek() + 2);
  }
  return HttpCode::OK;
}

HttpCode HttpRequest::parseContent() {
  m_body = std::string(m_buffer.peek(), static_cast<const char*>(m_buffer.beginWrite()));
  return HttpCode::OK;
}

HttpCode HttpRequest::parsePath() {
  if(m_url == "/") {
    m_path = "./static/index.html";
  } else if(m_url == "/index") {
    m_path = "./static/index.html";
  } else if(m_url == "/picture") {
    m_path = "./static/picture.html";
  } else if(m_url == "/video") {
    m_path = "./static/video.html";
  } else if(m_url == "/login") {
    m_path = "./static/login.html";
  } else if(m_url == "/register") {
    m_path = "./static/register.html";
  } else {
    m_path = "./static" + m_url;
  }
  return HttpCode::OK;
}

HttpCode HttpRequest::parseParams() {
  // todo
  return HttpCode::OK;
}

bool HttpRequest::isKeepAlive() {
  auto it = m_headers.find("Connection");
  if(it != m_headers.end()) {
    if(it->second == "Keep-Alive") {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

void HttpRequest::resetParse() {
  m_buffer.retrieveAll();
  m_parseStatus = ParseStatus::RequestLine;
  m_method = Method::GET;
  m_version = Version::Unknow;
  m_headers.clear();
  m_params.clear();
}

std::string HttpRequest::getParam(const std::string& key) const {
  auto it = m_params.find(key);
  return it == m_params.end() ? "" : it->second;
}

std::string HttpRequest::getHeader(const std::string& key) const {
  auto it = m_headers.find(key);
  return it == m_headers.end() ? "" : it->second;
}

}
