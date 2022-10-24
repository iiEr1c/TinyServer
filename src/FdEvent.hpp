#pragma once
#include <cassert>
#include <functional>
#include <mutex>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>

namespace TinyNet {
class Reactor;

struct FdEvent {
  enum IOEvent { READ = EPOLLIN | EPOLLPRI | EPOLLRDHUP, WRITE = EPOLLOUT };
  FdEvent();
  FdEvent(Reactor *, int _fd = -1);
  ~FdEvent();
  void setReadCallback(std::function<void()>);
  void setWriteCallback(std::function<void()>);
  void setCloseCallback(std::function<void()>);
  void setErrorCallback(std::function<void()>);

  inline const std::function<void()> &getReadCallback() const {
    return read_callback;
  };

  inline const std::function<void()> getWriteCallback() const {
    return write_callback;
  }

  inline const std::function<void()> getCloseCallback() const {
    return close_callback;
  }

  inline const std::function<void()> getErrorCallback() const {
    return error_callback;
  }

  void addListenEvents(uint32_t);
  void addListenEvents(IOEvent event);
  void delListenEvents(IOEvent event);
  void delAllListenEvents();
  void updateToReactor();
  void unRegisterFromReactor();

  inline int getFd() { return fd; };

  void setFd(int);

  inline int getListeningEvents() { return listening_event; }

  inline Reactor *getReactor() { return reactor; }

  void setReactor(Reactor *r);
  void setNonblock();
  bool isNonblock();
  std::string eventToString(IOEvent event) {
    switch (event) {
    case IOEvent::READ:
      return std::string("READ");
    case IOEvent::WRITE:
      return std::string("WRITE");
    default:
      return {};
    }
  }

protected:
  Reactor *reactor;
  int fd{-1};

  uint32_t listening_event{0};
  std::function<void()> read_callback{noop_function};
  std::function<void()> write_callback{noop_function};
  std::function<void()> close_callback{noop_function};
  std::function<void()> error_callback{noop_function};

private:
  static inline std::function<void()> noop_function = [] {};
};
}; // namespace TinyNet