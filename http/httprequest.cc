#include "httprequest.h"

#include <algorithm>
#include <cstdio>
#include <cwctype>
#include <mysql/mysql.h>
#include <string>
#include <utility>
#include <iostream>
#include <assert.h>

#include "../log/log.h"
#include "../db/mysql.h"
#include "http.h"

namespace hayroc {

HttpRequest::HttpRequest() {
  m_method = Method::GET;
  m_parseStatus = ParseStatus::RequestLine;
}

HttpRequest::~HttpRequest() {

}

void HttpRequest::init() {
  m_buffer.retrieveAll();
  m_parseStatus = ParseStatus::RequestLine;
  m_method = Method::GET;
  m_version = Version::Http11;
  m_headers.clear();
  m_params.clear();
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
          httpCode = verify();
          return httpCode;
        }
        break;
      }
      default: {
        DEBUG("defalut internal server error " + std::to_string(static_cast<int>(m_parseStatus)));
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
  if(m_body.empty()) {
    return HttpCode::OK;
  }

  int num = 0;
  int i = 0, j = 0;
  int n = m_body.size();
  auto converHex = [](char ch) {
    if(ch >= 'A' && ch <= 'F') return char(ch -'A' + 10);
    if(ch >= 'a' && ch <= 'f') return char(ch -'a' + 10);
    return ch;
  };

  std::string key, value;
  for(; i < n; i++) {
    char ch = m_body[i];
    switch(ch) {
      case '=': {
        key = m_body.substr(j, i - j);
        j = i + 1;
        break;
      }
      case '+': {
        m_body[i] = ' ';
        break;
      }
      case '%': {
        num = converHex(m_body[i + 1]) * 16 + converHex(m_body[i + 2]);
        m_body[i + 2] = num % 10 + '0';
        m_body[i + 1] = num / 10 + '0';
        i += 2;
        break;
      }
      case '&': {
        value = m_body.substr(j, i - j);
        j = i + 1;
        m_params[key] = value;
        DEBUG("解析urlencode " + key + " " + value);
        break;
      }
      default: {
        break;
      }
    }
  }
  if(m_params.count(key) == 0 && j < i) {
    value = m_body.substr(j, i - j);
    m_params[key] = value;
  }
  return HttpCode::OK;
}

HttpCode HttpRequest::verify() {
  std::string username = m_params["username"];
  std::string password = m_params["password"];

  MYSQL* sql = nullptr;
  MysqlGuard getTempSqlConn(&sql);

  std::string query = "SELECT username, password FROM user WHERE username='" + username + "' LIMIT 1";
  std::string insert = "INSERT INTO user(username, password) VALUES('" + username + "', '" + password + "')";

  if(mysql_query(sql, query.data())) {
    return HttpCode::InternalServerError;
  }
  MYSQL_RES* sqlres = mysql_store_result(sql);
  int size = mysql_num_fields(sqlres);

  bool fagUser = false, fagPass = false;
  while(MYSQL_ROW row = mysql_fetch_row(sqlres)) {
    std::string tmp(row[1]);
    fagUser = true;
    if(password == tmp) {
      fagPass = true;
    }
  }

  if(m_url == "/login") {
    if(fagUser && fagPass) {
      m_path = "./static/welcome.html";
    } else {
      m_path = "./static/error.html";
    }
  } else if(m_url == "/register") {
    if(!fagUser) {
      if(mysql_query(sql, insert.data())) {
        m_path = "./static/error.html";
      } else {
        m_path = "./static/welcome.html";
      }
    } else {
      m_path = "./static/error.html";
    }
  } else {
    m_path = "./static/error.html";
  }

  return HttpCode::OK;
}

bool HttpRequest::isKeepAlive() {
  auto it = m_headers.find("Connection");
  if(it != m_headers.end()) {
    auto& str = it->second;
    std::transform(str.begin(), str.end(), str.begin(), std::towlower);
    if(str == "keep-alive") {
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
