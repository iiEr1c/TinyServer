#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

namespace TinyNet {

class IPv4Address {
public:
  IPv4Address(const std::string &ip, uint16_t port);
  IPv4Address(sockaddr_in addr);
  sockaddr *getSockAddr();

  auto getIp() const -> std::string {
    char buf[16];
    ::inet_ntop(AF_INET, &addr.sin_addr, buf, static_cast<socklen_t>(16));
    return std::string(buf);
  }

  inline auto getPort() const -> int { return ::ntohs(addr.sin_port); }

  inline auto getFamily() const -> sa_family_t { return addr.sin_family; }

  inline auto getSockaddr_in() const -> const sockaddr * {
    return reinterpret_cast<const sockaddr *>(&addr);
  }

  IPv4Address(const IPv4Address &rhs) : addr(rhs.addr) {}

  IPv4Address &operator=(const IPv4Address &rhs) {
    addr = rhs.addr;
    return *this;
  }

  IPv4Address(IPv4Address &&rhs) : addr(rhs.addr) {}

  IPv4Address &operator=(IPv4Address &&rhs) {
    addr = rhs.addr;
    return *this;
  }

private:
  sockaddr_in addr;
};
} // namespace TinyNet