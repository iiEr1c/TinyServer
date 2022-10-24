#include "HttpResponse.hpp"
#include <logger.hpp>
#include <string_view>

namespace TinyNet {

HttpResponse::HttpResponse(bool close)
    : m_status{HttpStatus::STATUS_CODE_INIT}, m_closeConn{close} {}

HttpResponse::~HttpResponse() {}

void HttpResponse::appendToBuffer(std::string &output) const {
  using namespace std::string_view_literals;
  output.reserve(1024);
  output += "HTTP/1.1 "sv;
  output += std::to_string(m_status);
  output += " "sv;
  output += m_statusMsg;
  output += "\r\n"sv;

  if (m_closeConn) {
    output += "Connection: close\r\n"sv;
  } else {
    output += "Content-Length: "sv;
    output += std::to_string(m_body.size());
    output +=
        "\r\nConnection: Keep-Alive\r\nkeep-alive: max=6, timeout=60\r\n"sv;
  }

  for (const auto &header : m_headers) {
    output += header.first;
    output += ": "sv;
    output += header.second;
    output += "\r\n"sv;
  }
  output += "\r\n"sv;
  if (m_body.size() > 0) {
    output += m_body;
  }
  LDebug("HttpResponse::appendToBuffer: output buffer[{}]", output);
}

}; // namespace TinyNet