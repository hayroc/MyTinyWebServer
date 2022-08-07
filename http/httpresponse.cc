#include "httpresponse.h"

#include <asm-generic/errno-base.h>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <string>
#include <iostream>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "http.h"
#include "../log/log.h"

namespace hayroc {


HttpResponse::HttpResponse() : m_file(nullptr) {
  m_httpCode = HttpCode::OK;
  iov[0].iov_len = 0;
  iov[1].iov_len = 0;
}

HttpResponse::~HttpResponse() {

}

int HttpResponse::send(int fd, int &errorNum) {
  int size = toSendSize();
  while(true) {
    ssize_t size = writev(fd, iov, 2);
    if(size <= -1) {
      ERROR("回复报文失败 " + strerror(errno) + " " + std::to_string(toSendSize()));
      if(errno == EAGAIN) {
        errorNum = errno;
        return -1;
      }
      unmap();
      return -1;
    }

    if(toSendSize() == 0) {
      break;
    } else if(size > iov[0].iov_len) {
      iov[1].iov_base = (uint8_t*)iov[1].iov_base + (size - iov[0].iov_len);
      iov[1].iov_len -= (size - iov[0].iov_len);
      if(iov[0].iov_len) {
        m_buffer.retrieveAll();
        iov[0].iov_len = 0;
      }
    } else {
      iov[0].iov_base = (uint8_t*)iov[0].iov_base + size;
      iov[0].iov_len -= size;
      m_buffer.retrieve(size);
    }
  }
  return size;
}

void HttpResponse::makeResponse() {
  DEBUG("http code is " + getHttpCodeTitle());
  makeBody();
  appendResponseLine();
  appendHeader();
  appendBody();

  iov[0].iov_base = (void*)(m_buffer.peek());
  iov[0].iov_len = m_buffer.readableBytes();
  iov[1].iov_base = m_file;
  iov[1].iov_len = m_fileStat.st_size;
  DEBUG(m_buffer.peek());
}

std::string HttpResponse::getHttpCodeTitle() {
  switch(m_httpCode) {
    case HttpCode::OK: return "OK";
    case HttpCode::BadRequest: return "Bad Request";
    case HttpCode::Forbidden: return "Forbidden";
    case HttpCode::NotFound: return "Not Found";
    case HttpCode::MethodNotAllowed: return "Method Not Allowed";
    case HttpCode::RequestTimeOut: return "Request Time-out";
    case HttpCode::InternalServerError: return "Internal Server Error";
    default: return "";
  }
}

std::string HttpResponse::getHttpCodeContent() {
  static const char* badRequest = "Your browser sent a request that this server could not understand.\nThe request line contained invalid characters following the protocol string.";
  static const char* forbidden = "Sorry, access to this page is forbidden.";
  static const char* notFound = "The requested URL was not found on this server.";
  static const char* methodNotAllowed = "Access Denied";
  static const char* requestTimeOut = "Request Timeout";
  static const char* internalServerError = "There was an unusual problem serving the request file.";

  switch(m_httpCode) {
    case HttpCode::BadRequest: return badRequest;
    case HttpCode::Forbidden: return forbidden;
    case HttpCode::NotFound: return notFound;
    case HttpCode::MethodNotAllowed: return methodNotAllowed;
    case HttpCode::RequestTimeOut: return requestTimeOut;
    case HttpCode::InternalServerError: return internalServerError;
    default: return "";
  }
}


void HttpResponse::appendResponseLine() {
  m_buffer.append("HTTP/1.1 " + std::to_string(static_cast<int>(m_httpCode)) + " " + getHttpCodeTitle() + "\r\n");
}

void HttpResponse::appendHeader() {
  std::string date = "Date: " + getHttpTime() + "\r\n";
  std::string server = "Server: hayroc-pc\r\n";
  std::string contentLenght = "Content-Length: " + std::to_string(m_fileStat.st_size) + "\r\n";
  std::string contentType = "Content-Type: " + getFileType() + "; charset=utf8\r\n";
  std::string connetion;
  if(m_keepAlive) {
    connetion = "Connection: Keep-Alive\r\n";
    connetion += "Keep-Alive: timeout=" + std::to_string(TIMEOUT) + "\r\n";
  } else {
    connetion = "Connection: close\r\n";
  }

  m_buffer.append(date);
  m_buffer.append(server);
  m_buffer.append(contentLenght);
  m_buffer.append(contentType);
  m_buffer.append(connetion);
  m_buffer.append("\r\n");
}

void HttpResponse::appendBody() {
  // m_buffer.append(m_file, m_fileStat.st_size);
}

void HttpResponse::makeBody() {
  checkPath();
  stat(m_path.data(), &m_fileStat);

  int fd = open(m_path.data(), O_RDONLY);
  INFO("open " + m_path + " size: " + std::to_string(m_fileStat.st_size) + " fd: " + std::to_string(fd));
  if(fd < 0) ERROR("cant open file " + m_path);

  m_file = (char*)mmap(0, m_fileStat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
}

void HttpResponse::checkPath() {
  if(m_httpCode == HttpCode::OK) {
    if(stat(m_path.data(), &m_fileStat) < 0) {
      m_httpCode = HttpCode::NotFound;
    } else if(S_ISDIR(m_fileStat.st_mode)) {
      m_httpCode = HttpCode::NotFound;
    } else if(!(m_fileStat.st_mode & S_IROTH)) {
      m_httpCode = HttpCode::Forbidden;
    }
  }

  if(m_httpCode == HttpCode::BadRequest) {
    m_path = "./static/400.html";
  } else if(m_httpCode == HttpCode::Forbidden) {
    m_path = "./static/403.html";
  } else if(m_httpCode == HttpCode::NotFound) {
    m_path = "./static/404.html";
  } else if(m_httpCode == HttpCode::MethodNotAllowed) {
    m_path = "./static/405.html";
  } else if(m_httpCode == HttpCode::RequestTimeOut) {
    m_path = "./static/408.html";
  } else if(m_httpCode == HttpCode::InternalServerError) {
    m_path = "./static/500.html";
  }

}

void HttpResponse::init() {
  m_params.clear();

  if(m_file) { unmap(); }
  bzero(&m_fileStat, sizeof(m_fileStat));
}

void HttpResponse::unmap() {
  if(m_file) {
    munmap(m_file, m_fileStat.st_size);
    m_file = nullptr;
  }
}

std::string HttpResponse::getHttpTime() {
  char tstr[30];
  const char* fmt = "%a, %d %b %Y %H:%M:%S GMT";

  time_t now = time(nullptr);
  tm* gmt = gmtime(&now);
  strftime(tstr, sizeof(tstr), fmt, gmt);
  return tstr;
}

std::string HttpResponse::getFileType() {
  auto idx = m_path.find_last_of('.');
  if(idx == std::string::npos) {
    return "text/plain";
  }

  std::string type = m_path.substr(idx);
  if(type == ".html") {
    return "text/html";
  } else if(type == ".xml") {
    return "text/xml";
  } else if(type == ".xhtml") {
    return "application/xhtml+xml";
  } else if(type == ".txt") {
    return "text/plain";
  } else if(type == ".rtf") {
    return "application/rtf";
  } else if(type == ".pdf") {
    return "application/pdf";
  } else if(type == ".word") {
    return "application/nsword";
  } else if(type == ".png") {
    return "image/png";
  } else if(type == ".gif") {
    return "image/gif";
  } else if(type == ".jpg") {
    return "image/jpeg";
  } else if(type == ".jpeg") {
    return "image/jpeg";
  } else if(type == ".au") {
    return "audio/basic";
  } else if(type == ".mpeg") {
    return "video/mpeg";
  } else if(type == ".mpg") {
    return "video/mpeg";
  } else if(type == ".avi") {
    return "video/x-msvideo";
  } else if(type == ".gz") {
    return "application/x-gzip";
  } else if(type == ".tar") {
    return "application/x-tar";
  } else if(type == ".css") {
    return "text/css";
  } else if(type == ".js") {
    return "text/javascript";
  } else {
    return "text/plain";
  }
}

}
