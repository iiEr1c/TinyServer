#pragma once
#include <reactor.hpp>
#include <tcp/acceptor.hpp>
#include <tcp/io_thread_pool.hpp>

namespace TinyNet {

class TcpServer {
public:
  TcpServer(Reactor *loop, const IPv4Address &listenAddr, int size = 8);
  ~TcpServer();

  void start();

  void setMsgCallback(std::function<void(TcpConnection *, TcpBuffer *)> cb) {
    msgCallback = std::move(cb);
  }

  inline auto getMsgCallback() { return msgCallback; }

  void setonConnSetProtoCallback(std::function<void(TcpConnection *)> cb) {
    onConnSetProtoCallback = std::move(cb);
  }

  inline auto getonConnSetProtoCallback() { return onConnSetProtoCallback; }

  auto getIp() { return acceptor.getIp(); }

  auto getPort() { return acceptor.getPort(); }

private:
  void newConnection(int sockfd, IPv4Address &&peer);

private:
  Reactor *reactor;
  IOThreadPool io_pool;
  Acceptor acceptor;
  IPv4Address localAddr;
  std::function<void(TcpConnection *, TcpBuffer *)> msgCallback;
  std::function<void(TcpConnection *)> onConnSetProtoCallback;
  int tcp_count{0};
};
}; // namespace TinyNet