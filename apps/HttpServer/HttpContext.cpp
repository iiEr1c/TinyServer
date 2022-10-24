#include "HttpContext.hpp"
#include <algorithm>
#include <cassert>
#include <charconv>
#include <logger.hpp>
#include <tcp/tcp_buffer.hpp>
#include <timer.hpp>

namespace TinyNet {

HttpContext::HttpContext() : m_state(HttpRequestParseState::ExpectStartLine) {}

bool HttpContext::parseRequest(TcpBuffer *buf) {
  bool parseState = true;
  bool hasMoreByte = true;
  const char crlf[] = {'\r', '\n'};
  while (hasMoreByte) {
    if (m_state == HttpRequestParseState::ExpectStartLine) {
      const char *bufStart = buf->readableRawPtr();
      const char *bufEnd = buf->writeableRawPtr();
      const char *crlfPos = std::search(
          bufStart, bufEnd, std::boyer_moore_horspool_searcher(crlf, crlf + 2));
      LDebug("HttpContext::parseRequest(): http line:[{}]",
             std::string(bufStart, crlfPos));
      if (crlfPos != buf->writeableRawPtr()) {
        parseState = parseRequestLine(bufStart, crlfPos);
        if (parseState) {
          buf->retrieve(crlfPos - bufStart + 2);
          LDebug("buf retrieve [{}] bytes", crlfPos - bufStart + 2);
          m_request.setRecvTime(getNowMs());
          m_state = HttpRequestParseState::ExpectHeaders;
        } else {
          hasMoreByte = false;
        }
      } else {
        hasMoreByte = false;
      }
    } else if (m_state == HttpRequestParseState::ExpectHeaders) {
      const char *bufStart = buf->readableRawPtr();
      const char *bufEnd = buf->writeableRawPtr();
      const char *crlfPos = std::search(
          bufStart, bufEnd, std::boyer_moore_horspool_searcher(crlf, crlf + 2));
      if (crlfPos != bufEnd) {
        const char *colon = std::find(bufStart, crlfPos, ':');
        if (colon != crlfPos) {
          m_request.addHeader(bufStart, colon, crlfPos);
        } else {
          if (*bufStart == '\r' && *(bufStart + 1) == '\n') {
            if (m_request.getHeaders().count(std::string("Content-Length"))) {
              m_state = HttpRequestParseState::ExpectBody;
            } else {
              m_state = HttpRequestParseState::Done;
              hasMoreByte = false;
            }
          } else {
            parseState = false;
            hasMoreByte = false;
          }
        }
        buf->retrieve(crlfPos - bufStart + 2);
      }
    } else if (m_state == HttpRequestParseState::ExpectBody) {
      assert(m_request.getHeaders().count(std::string("Content-Length")) > 0);
      std::string str = m_request.getHeader("Content-Length");
      size_t len;
      std::string &body = m_request.getBody();
      if (auto [p, ec] =
              std::from_chars(str.data(), str.data() + str.size(), len);
          ec == std::errc()) {
        if (body.size() < len) {
          buf->readFromBuffer(body, len - body.size());
          if (body.size() == len) {
            m_state = HttpRequestParseState::Done;
          }
          hasMoreByte = false;
        }
      } else {
        parseState = false;
        hasMoreByte = false;
      }
    }
  }

  return parseState;
}

bool HttpContext::parseRequestLine(const char *begin, const char *end) {
  LDebug("HttpContext::parseRequestLine(): msg[{}]", std::string(begin, end));
  bool parseState = false;
  const char *start = begin;
  const char *space = std::find(start, end, ' ');
  if (space != end && m_request.setHttpMethod(start, space)) {
    start = space + 1;
    space = std::find(start, end, ' ');
    if (space != end) {
      const char *isQuery = std::find(start, space, '?');
      if (isQuery != space) {
        m_request.setTargetPath(start, isQuery);
        m_request.setTargetQuery(isQuery, space);
      } else {
        m_request.setTargetPath(start, space);
      }

      start = space + 1;
      parseState = end - start == 8 && std::equal(start, end, "HTTP/1.1");
      if (parseState) {
        m_request.setHttpVersion(HttpVersion::Http11);
      }
    }
  }
  return parseState;
}

HttpRequest &HttpContext::getRequest() { return m_request; }

void HttpContext::reset() {
  m_state = HttpRequestParseState::ExpectStartLine;
  HttpRequest dummy;
  m_request.swap(dummy);
}

bool HttpContext::isParseDone() {
  return m_state == HttpRequestParseState::Done;
}

}; // namespace TinyNet