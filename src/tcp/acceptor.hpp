#include <FdEvent.hpp>
#include <functional>
#include <tcp/ip_v4_address.hpp>

namespace TinyNet {

class IPv4Address;
class Reactor;

class Acceptor {
public:
  Acceptor(Reactor *reactor, const IPv4Address &listenAddr);

  ~Acceptor();

  void setNewConnectionCallback(
      const std::function<void(int sockfd, IPv4Address &&)> &cb) {
    newConnectionCallback = cb;
  }

  void listen();

  inline auto getIp() { return host_addr.getIp(); }

  inline auto getPort() { return host_addr.getPort(); }

private:
  void handleReadCallback();

  Reactor *reactor;
  std::function<void(int sockfd, IPv4Address &&)> newConnectionCallback;
  IPv4Address host_addr;
  FdEvent acceptFdEvent;
  int idleFd;
};
} // namespace TinyNet
