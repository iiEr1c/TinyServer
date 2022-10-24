#include "tcp_connection.hpp"
#include <iostream>
#include <logger.hpp>
#include <reactor.hpp>
#include <tcp/io_thread.hpp>

namespace TinyNet {

TcpConnection::TcpConnection(int _fd, IOThread *iothread,
                             const IPv4Address &peer_addr, int iothreadidx)
    : m_state{TcpConnectionState::Connected}, m_io_thread{iothread},
      m_peer_addr{peer_addr}, fd_event{iothread->getReactor(), _fd},
      m_index{iothreadidx} {
  fd_event.setReadCallback([this] { this->handleRead(); });
  fd_event.setWriteCallback([this] { this->handleWrite(); });
  fd_event.setCloseCallback([this] { this->handleClose(); });
  fd_event.setErrorCallback([this] { this->handleError(); });
  setKeepAlive(true); // keep alive enable
}

TcpConnection::~TcpConnection() { m_state = TcpConnectionState::NotConnected; }

void TcpConnection::handleRead() {
  if (m_state == TcpConnectionState::Connected) [[likely]] {
    int savedErrno = 0;
    ssize_t n = m_read_buffer.FromFdWriteBuffer(fd_event.getFd(), &savedErrno);
    if (n > 0) [[likely]] {
      LDebug(
          "TcpConnection::handleRead(): read fd[{}] {}:{}[{}] byte(s) message",
          fd_event.getFd(), m_peer_addr.getIp(), m_peer_addr.getPort(), n);
      msgCallback(this, std::addressof(m_read_buffer));
      if (m_state == TcpConnectionState::Connected) {
        auto slot = m_weak_slot_tcpconn.lock();
        if (slot != nullptr) {
          getIOThread()->getTimeWheel().fresh(std::move(slot));
        }
      }
    } else if (n == 0) {
      LDebug("TcpConnection::handleRead(): client fd[{}] {}:{}[{}] close conn ",
             fd_event.getFd(), m_peer_addr.getIp(), m_peer_addr.getPort(),
             stateToString(m_state));
      handleClose();
    } else {
      if (savedErrno == EINTR || savedErrno == EAGAIN ||
          savedErrno == EWOULDBLOCK) [[likely]] {
      } else {
        LError("TcpConnection::handleRead(): read client fd[{}] {}:{}[{}] [{}]",
               fd_event.getFd(), m_peer_addr.getIp(), m_peer_addr.getPort(),
               stateToString(m_state), strerror(savedErrno));
        handleError();
      }
    }
  }
}

void TcpConnection::handleWrite() {
  if (m_state == TcpConnectionState::Connected) [[likely]] {
    ssize_t n = ::write(fd_event.getFd(), m_write_buffer.readableRawPtr(),
                        m_write_buffer.readAble());
    if (n >= 0) [[likely]] {
      LDebug("TcpConnection::handleWrite(): write {}:{}[{}] [{}] bytes",
             getIp(), getPort(), stateToString(m_state), n);
      m_write_buffer.retrieve(static_cast<size_t>(n));
      if (m_write_buffer.readAble() == 0) {
        fd_event.delListenEvents(FdEvent::IOEvent::WRITE);
      }
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
        return;
      } else {
        LError("TcpConnection::handleWrite() Error, errno info[{}]",
               strerror(errno));
        handleClose();
      }
    }
  }
}

void TcpConnection::handleClose() {
  LDebug("TcpConnection::handleClose(): handle fd[{}] {}:{}[{}] close event",
         fd_event.getFd(), getIp(), getPort(), stateToString(m_state));
  m_state = TcpConnectionState::Closed;

  getCloseCallback()(this);
}

void TcpConnection::handleError() {
  int error = 0;
  socklen_t errlen = sizeof(error);

  if (::getsockopt(fd_event.getFd(), SOL_SOCKET, SO_ERROR, &error, &errlen) ==
      0) {
    LError("TcpConnection::handleError(): fd[{}] {}:{} error = [{}]",
           fd_event.getFd(), getIp(), getPort(), strerror(error));
  }
  m_state = TcpConnectionState::Closed;
  getCloseCallback()(this);
}

void TcpConnection::setKeepAlive(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(fd_event.getFd(), SOL_SOCKET, SO_KEEPALIVE, &optval,
               static_cast<socklen_t>(sizeof(optval)));
}

void TcpConnection::send(std::span<char> sp) {
  if (m_state != TcpConnectionState::Connected) [[unlikely]] {
    return;
  }

  ssize_t nwrote = 0;
  size_t remaining = sp.size();
  bool fault = false;

  if (m_write_buffer.readAble() == 0) {
    nwrote = ::write(fd_event.getFd(), sp.data(), sp.size());
    if (nwrote >= 0) {
      remaining = sp.size() - static_cast<size_t>(nwrote);
    } else {
      nwrote = 0;
      if (errno == EINTR || errno == EAGAIN || EWOULDBLOCK) [[likely]] {
      } else if (errno == EPIPE || errno == ECONNRESET) {
        LError("TcpConnection::send error, fd[{}]{}:{}", fd_event.getFd(),
               getIp(), getPort());
        fault = true;
      }
    }
  }

  if (!fault && remaining > 0) {
    LDebug("write not complete, remain [{}] bytes", remaining);
    m_write_buffer.writeToBuffer(sp.data() + nwrote, remaining);
    fd_event.addListenEvents(FdEvent::IOEvent::WRITE);
  }
}

void TcpConnection::enableReadEvent() {
  fd_event.addListenEvents(FdEvent::IOEvent::READ);
}

void TcpConnection::shutdownConn() {
  if (m_state != TcpConnectionState::Connected) {
    LError("tcp conn {}:{}[{}] already closed", getIp(), getPort(),
           stateToString(m_state));
    return;
  }

  LDebug("TcpConnection::shutdownConn():conn {}:{}[{}]", getIp(), getPort(),
         stateToString(m_state));
  ::shutdown(fd_event.getFd(), SHUT_RDWR);
  m_state = HalfClosing;

  fd_event.delListenEvents(FdEvent::IOEvent::WRITE);
}

void TcpConnection::registerToTimeWheel() {

  auto cb = [](const std::shared_ptr<TcpConnection> &conn) {
    if (conn->getState() == TcpConnectionState::Connected) {
      conn->shutdownConn();
    }
  };

  auto slot =
      std::make_shared<Slot<TcpConnection>>(shared_from_this(), std::move(cb));
  m_weak_slot_tcpconn = slot;
  LDebug("TcpConnection::registerToTimeWheel(): slot ref[{}]",
         slot.use_count());
  m_io_thread->getTimeWheel().fresh(std::move(slot));
}

}; // namespace TinyNet