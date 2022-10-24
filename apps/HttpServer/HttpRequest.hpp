#pragma once
#include "HttpCommon.hpp"
#include <map>
#include <string>

/**
 * @brief 一个http request包括
 * Request line/HTTP headers/Message body
 * https://www.ibm.com/docs/en/cics-ts/5.3?topic=protocol-http-requests
 * https://developer.mozilla.org/zh-CN/docs/Web/HTTP/Messages#http_%E8%AF%B7%E6%B1%82
 * https://developer.mozilla.org/en-US/docs/Web/HTTP/Messages#http_requests
 */
namespace TinyNet {

class HttpRequest {
public:
  struct HttpRequestTarget {
    std::string path;
    std::string query;
  };

  HttpRequest();
  ~HttpRequest();

  bool setHttpMethod(const char *start, const char *end);
  HttpMethod getHttpMethod() const;

  void setHttpVersion(HttpVersion);
  HttpVersion getHttpVersion() const;

  void setTargetPath(const char *start, const char *end);
  const std::string &getTargetPath() const;

  void setTargetQuery(const char *start, const char *end);
  const std::string &getTargetQuery() const;

  void setRecvTime(int64_t t);
  int64_t getRecvTime() const;

  void addHeader(const char *start, const char *colon, const char *end);
  std::string getHeader(const std::string &) const;
  std::map<std::string, std::string> &getHeaders();

  std::string &getBody();

  void swap(HttpRequest &rhs);

  void ForDebug();

private:
  HttpMethod m_method;
  HttpVersion m_version;
  HttpRequestTarget m_request_target;
  int64_t m_recv_time;
  std::map<std::string, std::string> m_headers;
  std::string m_body;
};
}; // namespace TinyNet