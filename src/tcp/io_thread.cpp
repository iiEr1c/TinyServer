#include "io_thread.hpp"
#include "tcp_connection.hpp"
#include "tcp_connection_wheel.hpp"
#include <iostream>
#include <logger.hpp>
#include <thread>

namespace TinyNet {

IOThread::IOThread(int idx)
    : m_reactor(new Reactor()), m_thr{}, m_tcp_timewheel(m_reactor, 16, 4),
      m_tcpconn_pool{}, m_clients{}, m_index{idx} {}

IOThread::~IOThread() {
  if (m_reactor == nullptr) {
    return;
  }
  m_reactor->stop();
  m_thr.join();
}

void IOThread::start() {
  m_thr = std::thread([this] { this->getReactor()->loop(); });
}

bool IOThread::addClient(
    int fd, IPv4Address peer,
    std::function<void(TcpConnection *)> setProtoCallback,
    std::function<void(TcpConnection *, TcpBuffer *)> msgCallback) {
  LDebug("IOThread::addClient: ^^^^watch^^^^this^^^^thread^^^^^id");
  auto it = m_clients.find(fd);
  if (it != m_clients.end()) [[unlikely]] {
    // 已经在存在该TcpConnection
    // todo: 修改此处代码, 按照状态机, 这里事实上不应该进入
    std::shared_ptr<TcpConnection> &conn = it->second;
    if (conn != nullptr &&
        conn->getState() != TinyNet::TcpConnectionState::Closed) {
      LError("IOThread::addClient: conn != nullptr && state[{}] != Closed[{}]",
             conn->getState(), TinyNet::TcpConnectionState::Closed);
      return false;
    }
    LDebug("already exist!!! todo!!! close this connection!!!");
    if (conn != nullptr) {
      LDebug("conn != nullptr, close!!!~~~");
      ::close(conn->getFd());
      m_clients.erase(it);
    }
    // todo todo todo
    // todo
  } else {
    TcpConnection *conn = m_tcpconn_pool.acquire(fd, this, peer, getIndex());
    if (conn == nullptr) [[unlikely]] {
      LError("IOThread::addClient(): 连接数到达上限[8192]");
      // close fd
      ::close(fd);
    } else {
      conn->setMsgCallback(std::move(msgCallback));
      conn->setCloseCallback(
          [this](TcpConnection *tcpconn) { this->removeConnection(tcpconn); });
      conn->enableReadEvent();
      setProtoCallback(conn);
      auto shared_conn =
          std::shared_ptr<TcpConnection>(conn, [this](TcpConnection *tcpconn) {
            LDebug("tcp conn {}:{} disconnected", tcpconn->getIp(),
                   tcpconn->getPort());
            this->getObjectPool().release(tcpconn);
          });

      shared_conn->registerToTimeWheel();
      m_clients.emplace(fd, std::move(shared_conn));

      LDebug("IOThread::addClient(): tcp conn {}:{}, ref count = [{}]",
             m_clients[fd]->getIp(), m_clients[fd]->getPort(),
             m_clients[fd].use_count());
    }
  }

  return true;
}

void IOThread::removeConnection(TcpConnection *conn) {
  conn->getFdEvent().unRegisterFromReactor();
  ::close(conn->getFdEvent().getFd());
  LDebug("IOThread::removeConnection: remove client {}:{}, fd[{}] state[{}]",
         conn->getIp(), conn->getPort(), conn->getFdEvent().getFd(),
         m_clients.count(conn->getFdEvent().getFd()) > 0
             ? std::string("success")
             : std::string("failure"));
  [[maybe_unused]] size_t n = m_clients.erase(conn->getFd());
}
}; // namespace TinyNet