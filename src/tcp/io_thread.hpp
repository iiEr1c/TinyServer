#pragma once

#include <map>
#include <memory>
#include <object_pool.hpp>
#include <reactor.hpp>
#include <tcp/ip_v4_address.hpp>
#include <thread>
#include <vector>

namespace TinyNet {

class TcpServer;
class TcpConnection;
class TcpTimeWheel;

class IOThread {
public:
  explicit IOThread(int);
  ~IOThread();
  IOThread(const IOThread &) = delete;
  IOThread &operator=(const IOThread &) = delete;
  IOThread(IOThread &&rhs)
      : m_reactor(rhs.m_reactor), m_thr(std::move(rhs.m_thr)),
        m_tcp_timewheel(std::move(rhs.m_tcp_timewheel)),
        m_clients(std::move(rhs.m_clients)) {
    rhs.m_reactor = nullptr;
  }
  IOThread &operator=(IOThread &&rhs) {
    m_reactor = rhs.m_reactor;
    m_thr = std::move(rhs.m_thr);
    m_clients = std::move(rhs.m_clients);
    rhs.m_reactor = nullptr;
    return *this;
  }
  inline Reactor *getReactor() { return m_reactor; }

  inline TcpTimeWheel &getTimeWheel() { return m_tcp_timewheel; }

  inline auto &getObjectPool() { return m_tcpconn_pool; }

  inline int getIndex() { return m_index; }

  bool addClient(int fd, IPv4Address, std::function<void(TcpConnection *)>,
                 std::function<void(TcpConnection *, TcpBuffer *)>);

  void start();

private:
  void removeConnection(TcpConnection *);

private:
  Reactor *m_reactor;
  std::thread m_thr;
  TcpTimeWheel m_tcp_timewheel;
  ObjectPool<TcpConnection, 8192> m_tcpconn_pool;
  std::map<int, std::shared_ptr<TcpConnection>> m_clients;
  int m_index;
};

}; // namespace TinyNet