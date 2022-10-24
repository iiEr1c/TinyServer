#pragma once
#include "HttpRequest.hpp"

namespace TinyNet {

class TcpBuffer;

class HttpContext {
public:
  enum class HttpRequestParseState {
    ExpectStartLine,
    ExpectHeaders,
    ExpectBody,
    Done
  };

  HttpContext();
  bool parseRequest(TcpBuffer *);

  HttpRequest &getRequest();

  void reset();

  bool isParseDone();

private:
  bool parseRequestLine(const char *begin, const char *end);

private:
  HttpRequestParseState m_state;
  HttpRequest m_request;
};
}; // namespace TinyNet