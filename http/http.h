#pragma once

namespace hayroc {

enum class Method {
  Invalid, GET, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATH
};

enum class HttpCode {
  Continue            = 100,
  OK                  = 200,
  BadRequest          = 400,
  Forbidden           = 403,
  NotFound            = 404,
  MethodNotAllowed    = 405,
  RequestTimeOut      = 408,
  InternalServerError = 500
};

};
