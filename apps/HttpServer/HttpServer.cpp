#include "HttpServer.hpp"
#include "HttpContext.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "TinyJson/include/TinyJson.hpp"
#include <logger.hpp>
#include <span>
#include <tcp/tcp_connection.hpp>

namespace TinyNet {

void defaultHttpCallback(const HttpRequest &req, HttpResponse *respon) {
  respon->setStatusCode(HttpStatus::STATUS_CODE_200);
  respon->setStatusMsg("OK");
}

HttpServer::HttpServer(Reactor *reactor, const IPv4Address &listenAddr,
                       int threadNum)
    : m_http_server(reactor, listenAddr, threadNum),
      m_httpCallback{defaultHttpCallback}, m_httpRouter(threadNum) {

  m_http_server.setonConnSetProtoCallback(
      [this](TcpConnection *conn) { this->onConnSetProtocol(conn); });
  m_http_server.setMsgCallback(
      [this](TinyNet::TcpConnection *conn, TinyNet::TcpBuffer *buf) {
        this->onMessage(conn, buf);
      });
  for (int i = 0; i < threadNum; ++i) {
    /* 暂时只支持GET方法 */
    m_httpRouter[i].add<HttpMethod::GET>(
        "/", [](std::pair<TcpConnection *, HttpResponse *> pair, auto &param) {
          pair.second->setStatusCode(HttpStatus::STATUS_CODE_200);
          pair.second->setStatusMsg("OK");
          pair.second->addHeader("Content-Type", "text/html; charset=UTF-8");
          pair.second->setBody(std::string("<html>"
                                           "<head>"
                                           "<title> WebServer </title>"
                                           "</head>"
                                           "<body>"
                                           "<p> Hello World </p>"
                                           "<p> 0xFF </p>"
                                           "<p> 0xFE </p>"
                                           "</body>"
                                           "</html>"));
        });

    m_httpRouter[i].add<HttpMethod::GET>(
        "/404.html",
        [](std::pair<TcpConnection *, HttpResponse *> pair, auto &param) {
          pair.second->setStatusCode(HttpStatus::STATUS_CODE_404);
          pair.second->setStatusMsg("Not Found");
          pair.second->addHeader("Content-Type", "text/html; charset=UTF-8");
          pair.second->setBody(
              std::string("<html>"
                          "<head><title>404 Not Found</title></head>"
                          "<body>"
                          "<center><h1>404 Not Found</h1></center>"
                          "<hr><center> TinyServer </center>"
                          "</body>"
                          "</html>"));
        });

    m_httpRouter[i].add<HttpMethod::GET>(
        "/user/:userId",
        [](std::pair<TcpConnection *, HttpResponse *> pair, auto &param) {
          // 省略搜索数据库
          pair.second->setStatusCode(HttpStatus::STATUS_CODE_200);
          pair.second->setStatusMsg("OK");
          pair.second->addHeader("Content-Type", "application/json");
          TinyJson::JsonValue val;
          val.root->type = TinyJson::JsonType::JSON_OBJECT;
          val.root->objSize = 2;
          val.root->object =
              (TinyJson::KVnode *)::malloc(sizeof(TinyJson::KVnode) * 2);

          /* 生成{"userId":"123456"}这样的串 */
          char *userId = (char *)::malloc(6);
          userId[0] = 'u';
          userId[1] = 's';
          userId[2] = 'e';
          userId[3] = 'r';
          userId[4] = 'I';
          userId[5] = 'd';
          char *userIdVal = (char *)::malloc(param[0].length);
          size_t len = 0;
          if (param[0].data[0] != '%') {
            for (size_t j = 0; j < param[0].length; ++j) {
              userIdVal[j] = param[0].data[j];
            }
          } else {
            auto parse_hex = [](char ch) -> unsigned char {
              if (ch >= '0' && ch <= '9') {
                return static_cast<unsigned char>(ch - '0');
              } else if (ch >= 'A' && ch <= 'F') {
                return static_cast<unsigned char>(ch - 'A' + 10);
              } else if (ch >= 'a' && ch <= 'f') {
                return static_cast<unsigned char>(ch - 'a' + 10);
              } else {
                return 0;
              }
            };
            for (size_t j = 0; j < param[0].length;) {
              userIdVal[len] = static_cast<unsigned char>(
                  (parse_hex(param[0].data[j + 1]) << 4) +
                  parse_hex(param[0].data[j + 2]));
              j += 3;
              ++len;
            }
          }

          TinyJson::Node *v =
              (TinyJson::Node *)::malloc(sizeof(TinyJson::Node));
          v->type = TinyJson::JsonType::JSON_STRING;
          v->str = userIdVal;
          v->strLen = len;

          val.root->object[0].str = userId;
          val.root->object[0].strLen = 6;
          val.root->object[0].v = v;

          char *ip = (char *)malloc(8);
          ip[0] = 'i';
          ip[1] = 'p';
          std::string ipAddr = pair.first->getIp();
          char *ipaddr = (char *)malloc(ipAddr.size());
          for (size_t j = 0; j < ipAddr.size(); ++j) {
            ipaddr[j] = ipAddr[j];
          }

          TinyJson::Node *v2 =
              (TinyJson::Node *)::malloc(sizeof(TinyJson::Node));
          v2->type = TinyJson::JsonType::JSON_STRING;
          v2->str = ipaddr;
          v2->strLen = ipAddr.size();
          val.root->object[1].str = ip;
          val.root->object[1].strLen = 2;
          val.root->object[1].v = v2;

          TinyJson json;
          std::string body = json.stringify2(&val);
          pair.second->setBody(std::move(body));
        });
  }
}

void HttpServer::onConnSetProtocol(TcpConnection *conn) {
  LDebug("HttpServer::onConnSetProtocol() : {}:{}[{}]", conn->getIp(),
         conn->getPort(), conn->stateToString(conn->getState()));
  *conn->getContext() = HttpContext();
}

void HttpServer::start() {
  LWarn("HttpServer {}:{} start", m_http_server.getIp(),
        m_http_server.getPort());
  m_http_server.start();
}

void HttpServer::onMessage(TcpConnection *conn, TcpBuffer *buf) {

  HttpContext *context = std::any_cast<HttpContext>(conn->getContext());
  if (!context->parseRequest(buf)) {
    std::string response("HTTP/1.1 400 Bad Request\r\n\r\n");
    conn->send(std::span<char>(response.begin(), response.end()));
    LDebug("parseError, shutdown^^^^^^^^^^^^^^");
    conn->shutdownConn();
  }

  if (context->isParseDone()) {
    onRequest(conn, context->getRequest());
    context->reset();
  }
}

void HttpServer::onRequest(TcpConnection *conn, const HttpRequest &req) {
  const std::string connection = req.getHeader("Connection");
  bool isClosed = connection == "close";
  HttpResponse res(isClosed);
  m_httpRouter[conn->getFakeThreadIndex()].route<HttpMethod::GET>(
      req.getTargetPath(), std::make_pair(conn, std::addressof(res)));
  std::string responseMsg;
  res.appendToBuffer(responseMsg);
  conn->send(std::span<char>(responseMsg.begin(), responseMsg.end()));
  if (res.closeConnection()) {
    conn->shutdownConn();
    LDebug("connection: close. shutdown~~~~~~~~~~~~~~~~~~~~~~~~~");
  }
}

}; // namespace TinyNet