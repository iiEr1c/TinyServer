#pragma once
#include <FdEvent.hpp>
#include <any>
#include <functional>
#include <memory>
#include <slot.hpp>
#include <span>
#include <string>
#include <tcp/ip_v4_address.hpp>
#include <tcp/tcp_buffer.hpp>

namespace TinyNet {

class IOThread;
class TcpBuffer;

enum TcpConnectionState { NotConnected, Connected, HalfClosing, Closed };

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
  TcpConnection(int fd, IOThread *, const IPv4Address &, int iothreadidx);
  ~TcpConnection();

  void setKeepAlive(bool on);

  inline TcpConnectionState getState() const { return m_state; }

  inline int getFd() { return fd_event.getFd(); }

  inline auto getIp() { return m_peer_addr.getIp(); }

  inline auto getPort() { return m_peer_addr.getPort(); }

  inline FdEvent &getFdEvent() { return fd_event; }

  inline IOThread *getIOThread() { return m_io_thread; }

  void
  setMsgCallback(const std::function<void(TcpConnection *, TcpBuffer *)> &cb) {
    msgCallback = cb;
  }

  std::function<void(TcpConnection *)> getCloseCallback() {
    return closeCallback;
  }

  void setCloseCallback(const std::function<void(TcpConnection *)> &cb) {
    closeCallback = cb;
  }

  void send(std::span<char>);

  void enableReadEvent();

  void shutdownConn();

  void registerToTimeWheel();

  inline int getFakeThreadIndex() { return m_index; }

  inline std::any *getContext() { return std::addressof(m_context); }

  std::string stateToString(TcpConnectionState state) {
    // NotConnected, Connected, HalfClosing, Closed
    switch (state) {
    case NotConnected:
      return "NotConnected";
    case Connected:
      return "Connected";
    case HalfClosing:
      return "HalfClosing";
    case Closed:
      return "Closed";
    default:
      return "";
    }
  }

private:
  void handleRead();
  void handleWrite();
  void handleClose();
  void handleError();

private:
  TcpConnectionState m_state{TcpConnectionState::NotConnected};
  IOThread *m_io_thread{nullptr};
  IPv4Address m_peer_addr;
  TcpBuffer m_read_buffer;
  TcpBuffer m_write_buffer;
  FdEvent fd_event;
  std::weak_ptr<Slot<TcpConnection>> m_weak_slot_tcpconn;

  std::function<void(TcpConnection *, TcpBuffer *)> msgCallback;

  std::function<void(TcpConnection *)> closeCallback;
  std::any m_context;
  int m_index;
};

}; // namespace TinyNet