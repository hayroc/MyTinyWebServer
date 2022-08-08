#pragma once

#include <memory>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "http.h"
#include "../base/buffer.h"
#include "../log/log.h"
#include "httprequest.h"

namespace hayroc {

enum class HttpCode;

class HttpResponse {
public:
  using ptr = std::shared_ptr<HttpResponse>;
  static const int TIMEOUT = 10000;

  void init();
  void makeResponse();
  int send(int fd, int& errorNum);
  auto getPath() const { return m_path; }
  auto getHttpCode() const { return m_httpCode; }
  int toSendSize() const { return iov[0].iov_len + iov[1].iov_len; }
  bool isKeepAlive() const { return m_keepAlive; }
  void setPath(const std::string& path) { m_path = path; }
  void setHttpCode(HttpCode httpCode) { m_httpCode = httpCode; }
  void setKeepAlive(bool keepAlive) { m_keepAlive = keepAlive; }
  void setParams(Map params) { m_params.swap(params); }

private:
  std::string getHttpCodeTitle();
  std::string getHttpCodeContent();
  std::string getHttpTime();
  std::string getFileType();
  void appendResponseLine();
  void appendHeader();
  void appendBody();
  void makeBody();
  void checkPath();
  void unmap();

public:
  HttpResponse();
  ~HttpResponse();

private:
  Buffer m_buffer; // res header
  Map m_params;

  bool m_keepAlive;
  HttpCode m_httpCode;
  std::string m_path;

  iovec iov[2];

  char* m_file; // res body
  struct stat m_fileStat;
};

}
