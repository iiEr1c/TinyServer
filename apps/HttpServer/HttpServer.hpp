#pragma once
#include "HttpRouter.hpp"
#include <tcp/tcp_server.hpp>
#include <tuple>

namespace TinyNet {

class HttpRequest;
class HttpResponse;
class TcpConnection;

class HttpServer {
public:
  explicit HttpServer(Reactor *reactor, const IPv4Address &listenAddr,
                      int threadNum);

  HttpServer(const HttpServer &) = delete;
  HttpServer &operator=(const HttpServer &) = delete;

  /*
   * 移动构造和移动赋值的lambda需要重新设置那两个回调, 因为捕获的this可能会变
   */

  void start(); // 启动tcp server

private:
  void onMessage(TcpConnection *, TcpBuffer *);
  void onRequest(TcpConnection *, const HttpRequest &);

  /**
   * 当建立tcp连接时, 设置此次通信的协议
   */
  void onConnSetProtocol(TcpConnection *);

private:
  TcpServer m_http_server;
  std::function<void(const HttpRequest &, HttpResponse *)> m_httpCallback;
  std::vector<HttpRouter<std::pair<TcpConnection *, HttpResponse *>>> m_httpRouter;
};
}; // namespace TinyNet