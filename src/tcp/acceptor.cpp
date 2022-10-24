#include "acceptor.hpp"
#include "ip_v4_address.hpp"
#include <fcntl.h>
#include <iostream>
#include <logger.hpp>
#include <unistd.h>

namespace TinyNet {
Acceptor::Acceptor(Reactor *_reactor, const IPv4Address &listenAddr)
    : reactor(_reactor), host_addr(listenAddr),
      acceptFdEvent{_reactor,
                    ::socket(listenAddr.getFamily(),
                             SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                             IPPROTO_TCP)},
      idleFd{::open("/dev/null", O_RDONLY | O_CLOEXEC)} {

  // reuse addr
  int optval = 1;
  ::setsockopt(acceptFdEvent.getFd(), SOL_SOCKET, SO_REUSEADDR, &optval,
               static_cast<socklen_t>(sizeof(optval)));
  // reuse port
  ::setsockopt(acceptFdEvent.getFd(), SOL_SOCKET, SO_REUSEPORT, &optval,
               static_cast<socklen_t>(sizeof(optval)));

  // bind
  int ret = ::bind(acceptFdEvent.getFd(), host_addr.getSockaddr_in(),
                   sizeof(sockaddr_in));
  if (ret < 0) {
    LError("Acceptor::Acceptor: fd[{}] bind error", acceptFdEvent.getFd());
    std::abort();
  }

  acceptFdEvent.setReadCallback([this] { this->handleReadCallback(); });
  LDebug("Acceptor::Acceptor() init: acceptFd[{}], idleFd[{}], host: {}:{}",
         acceptFdEvent.getFd(), idleFd, listenAddr.getIp(),
         listenAddr.getPort());
};

Acceptor::~Acceptor() {
  acceptFdEvent.unRegisterFromReactor();
  ::close(acceptFdEvent.getFd());
  ::close(idleFd);
}

void Acceptor::listen() {
  int ret = ::listen(acceptFdEvent.getFd(), SOMAXCONN);
  if (ret < 0) {
    LError("Acceptor::Acceptor: fd[{}], {}:{} listen error",
           acceptFdEvent.getFd(), host_addr.getIp(), host_addr.getPort());
    std::abort();
  }
  acceptFdEvent.addListenEvents(FdEvent::IOEvent::READ);
}

void Acceptor::handleReadCallback() {
  sockaddr_in peeraddr;
  socklen_t len = sizeof(peeraddr);
  int connfd =
      ::accept4(acceptFdEvent.getFd(), reinterpret_cast<sockaddr *>(&peeraddr),
                &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (connfd < 0) [[unlikely]] {
    if (errno == EMFILE) {
      ::close(idleFd);
      idleFd = ::accept(acceptFdEvent.getFd(), nullptr, nullptr);
      ::close(idleFd);
      idleFd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
    LError("Acceptor::handleReadCallback(): {}:{} accept error[{}]", getIp(),
           getPort(), strerror(errno));
  } else {
    LDebug(
        "Acceptor::handleReadCallback(): hostFd[{}], peerFd[{}], peer=>{}:{}",
        acceptFdEvent.getFd(), connfd, IPv4Address{peeraddr}.getIp(),
        IPv4Address{peeraddr}.getPort());
    if (newConnectionCallback) [[likely]] {
      newConnectionCallback(connfd, IPv4Address{peeraddr});
    } else {
      ::close(connfd);
    }
  }
}

}; // namespace TinyNet