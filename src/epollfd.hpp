#pragma once
#include <fcntl.h>
#include <logger.hpp>
#include <unistd.h>

namespace TinyNet {
class EpollFd {
public:
  EpollFd(int fd) : m_fd(fd) {}
  ~EpollFd() {
    if (m_fd < 0) [[unlikely]] {
      return;
    }
    ::close(m_fd);
  }

  int getFd() const { return m_fd; }

private:
  int m_fd{-1};
};
}; // namespace TinyNet