#pragma once

#include <memory>
#include <map>
#include <string>
#include <unistd.h>

#include "http.h"
#include "../base/buffer.h"

namespace hayroc {

enum class Method;
enum class HttpCode;

using Map = std::map<std::string, std::string>;

class HttpRequest {
public:
  using ptr = std::shared_ptr<HttpRequest>;
  enum class ParseStatus : int {
    RequestLine = 0, Header = 1, Content = 2
  };

  enum class Version {
    Unknow, Http10, Http11
  };

  int recv(int fd, int &errorNum);
  HttpCode parse();
  bool isKeepAlive();
  void resetParse();

  std::string getPath() const { return m_path; }
  std::string getBody() const { return m_body; }
  std::string getParam(const std::string& key) const;
  std::string getHeader(const std::string& key) const;
  Map getParams() const { return m_params; }

private:
  HttpCode parseRequestLine();
  HttpCode parseHeader();
  HttpCode parseContent();
  HttpCode parsePath();
  HttpCode parseParams();

public:
  HttpRequest();
  ~HttpRequest();

private:
  Buffer m_buffer;
  ParseStatus m_parseStatus;

  std::string m_url;
  std::string m_path;
  std::string m_body;
  Method m_method;
  Version m_version;
  Map m_headers;
  Map m_params;
};

}

