#include "tcp_server.hpp"
#include "io_thread.hpp"
#include <logger.hpp>

namespace TinyNet {
TcpServer::TcpServer(Reactor *_reactor, const IPv4Address &listenAddr, int size)
    : reactor(_reactor), io_pool(size), acceptor(_reactor, listenAddr),
      localAddr(listenAddr) {

  acceptor.setNewConnectionCallback([this](int sockfd, IPv4Address &&peer) {
    this->newConnection(sockfd, std::move(peer));
  });
  LDebug("TcpServer init: {}:{}", listenAddr.getIp(), listenAddr.getPort());
}

TcpServer::~TcpServer() {}

void TcpServer::newConnection(int sockfd, IPv4Address &&peer) {
  IOThread *loop = io_pool.getIOThread();
  LDebug("TcpServer::newConnection: new connection fd[{}] from {}:{}", sockfd,
         peer.getIp(), peer.getPort());

  auto cb = [=, this] {
    loop->addClient(sockfd, peer, this->getonConnSetProtoCallback(),
                    this->getMsgCallback());
  };

  if (loop->getReactor()->addTaskCrossThread(std::move(cb))) [[likely]] {

    ++tcp_count;
    LDebug("TcpServer::newConnection: current tcp connection count[{}]",
           tcp_count);
  } else {

    ::close(sockfd);
    LError("add async task error, fd[{}]", sockfd);
  }
}

void TcpServer::start() {
  acceptor.listen();
  reactor->loop();
}
}; // namespace TinyNet