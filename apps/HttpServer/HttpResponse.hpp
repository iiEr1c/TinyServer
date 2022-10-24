#pragma once
#include "HttpCommon.hpp"
#include <map>
#include <string>

namespace TinyNet {
/*
 * https://developer.mozilla.org/en-US/docs/Web/HTTP/Messages#http_responses
 * 默认http1.1
 */
using namespace std::string_view_literals;
class HttpResponse {
public:
  explicit HttpResponse(bool);
  ~HttpResponse();

  void setStatusCode(HttpStatus statu) { m_status = statu; }

  void setStatusMsg(const std::string msg) { m_statusMsg = msg; }

  void addHeader(const std::string &key, const std::string &value) {
    m_headers[key] = value;
  }

  void setCloseConnection(bool on) { m_closeConn = on; }
  bool closeConnection() const { return m_closeConn; }

  void setContentType(const std::string &contentType) {
    addHeader("Content-Type", contentType);
  }

  void setBody(const std::string &body) { m_body = body; }

  void setBody(std::string&& body) { m_body = std::move(body); }

  void appendToBuffer(std::string &buf) const;

private:
  HttpStatus m_status;
  std::map<std::string, std::string> m_headers;
  std::string m_statusMsg; // 不同状态码返回的msg
  bool m_closeConn;
  std::string m_body;
};
}; // namespace TinyNet