#include "HttpRequest.hpp"
#include <logger.hpp>

namespace TinyNet {

HttpRequest::HttpRequest()
    : m_method{HttpMethod::INVALID}, m_version{HttpVersion::Unknown} {}

HttpRequest::~HttpRequest() {}

bool HttpRequest::setHttpMethod(const char *start, const char *end) {
  std::string method(start, end);

  // 这里先暂时只支持几种method
  if (method == "GET") {
    m_method = HttpMethod::GET;
  } else if (method == "POST") {
    m_method = HttpMethod::POST;
  } else if (method == "PUT") {
    m_method = HttpMethod::PUT;
  } else if (method == "DELETE") {
    m_method = HttpMethod::DELETE;
  } else {
    m_method = HttpMethod::INVALID;
  }
  return m_method != HttpMethod::INVALID;
}

HttpMethod HttpRequest::getHttpMethod() const { return m_method; }

void HttpRequest::setHttpVersion(HttpVersion v) { m_version = v; }

HttpVersion HttpRequest::getHttpVersion() const { return m_version; }

void HttpRequest::setTargetPath(const char *start, const char *end) {
  m_request_target.path.assign(start, end);
}

const std::string &HttpRequest::getTargetPath() const {
  return m_request_target.path;
}

void HttpRequest::setTargetQuery(const char *start, const char *end) {
  m_request_target.query.assign(start, end);
}

void HttpRequest::setRecvTime(int64_t t) { m_recv_time = t; }

int64_t HttpRequest::getRecvTime() const { return m_recv_time; }

void HttpRequest::addHeader(const char *start, const char *colon,
                            const char *end) {
  std::string key(start, colon);
  ++colon;
  while (colon < end && std::isspace(*colon)) {
    ++colon;
  }
  std::string value(colon, end);
  while (!value.empty() && std::isspace(value.back())) {
    value.pop_back();
  }
  LDebug("HttpRequest::addHeader(): kv pair = [{}]:[{}]", key, value);
  m_headers[key] = std::move(value);
}

std::string HttpRequest::getHeader(const std::string &key) const {
  std::string value;
  if (auto it = m_headers.find(key); it != m_headers.end()) {
    value = it->second;
  }
  return value;
}

std::map<std::string, std::string> &HttpRequest::getHeaders() {
  return m_headers;
}

std::string &HttpRequest::getBody() { return m_body; }

void HttpRequest::swap(HttpRequest &rhs) {
  std::swap(m_method, rhs.m_method);
  std::swap(m_version, rhs.m_version);
  std::swap(m_request_target.path, rhs.m_request_target.path);
  std::swap(m_request_target.query, rhs.m_request_target.query);
  std::swap(m_recv_time, rhs.m_recv_time);
  std::swap(m_headers, rhs.m_headers);
  std::swap(m_body, rhs.m_body);
}

void HttpRequest::ForDebug() {
  LDebug("HttpMethod:[{}]", m_method);
  LDebug("HttpVersion:[{}]", m_version);
  LDebug("HttpRequestTarget: path:[{}], query:[{}]", m_request_target.path,
         m_request_target.query);
  LDebug("recv time:[{}]", m_recv_time);
  for (const auto &header : m_headers) {
    LDebug("kv: key[{}], value[{}]", header.first, header.second);
  }
}

}; // namespace TinyNet