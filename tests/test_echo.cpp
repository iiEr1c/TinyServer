#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <tcp/tcp_server.hpp>

// static TinyNet::Log log;

class EchoServer {
public:
  EchoServer(TinyNet::Reactor *_reactor, const TinyNet::IPv4Address &listenAddr)
      : reactor(_reactor), server(_reactor, listenAddr) {
    server.setMsgCallback(
        [this](TinyNet::TcpConnection *conn, TinyNet::TcpBuffer *buf) {
          this->onMessage(conn, buf);
        });
  }

  void start() {
    server.start();
  }

private:
  void onMessage(TinyNet::TcpConnection *conn, TinyNet::TcpBuffer *buf) {
    std::vector<char> vec;
    buf->readFromBuffer(vec, buf->readAble()); // 将数据全部取完
    // 将数据返回给客户端
    // conn->send();
    conn->send(std::span<char>{vec.begin(), vec.end()});
  }
  TinyNet::Reactor *reactor;
  TinyNet::TcpServer server;
};

TEST_CASE("echo server", "[echo server]") {
    TinyNet::Reactor reactor;
    TinyNet::IPv4Address listenAddr("127.0.0.1", 8888);
    EchoServer server(&reactor, listenAddr);
    server.start();
}