#include "FdEvent.hpp"
#include "reactor.hpp"
#include <fcntl.h>
#include <logger.hpp>
#include <unistd.h>

namespace TinyNet {

FdEvent::FdEvent() {}

FdEvent::FdEvent(TinyNet::Reactor *_reactor, int _fd)
    : reactor(_reactor), fd(_fd) {
  if (_reactor == nullptr) {
    LError("FdEvent::FdEvent(TinyNet::Reactor *_reactor, int "
           "_fd): reactor can not nullptr");
    std::abort();
  }
  if (_fd < 0) {
    LError(
        "FdEvent::FdEvent(TinyNet::Reactor *_reactor, int _fd): fd can not -1");
    std::abort();
  }
}

FdEvent::~FdEvent() {}

void FdEvent::setReadCallback(std::function<void()> cb) {
  read_callback = std::move(cb);
}

void FdEvent::setWriteCallback(std::function<void()> cb) {
  write_callback = std::move(cb);
}

void FdEvent::setCloseCallback(std::function<void()> cb) {
  close_callback = std::move(cb);
}

void FdEvent::setErrorCallback(std::function<void()> cb) {
  error_callback = std::move(cb);
}

void FdEvent::addListenEvents(uint32_t event) {
  if (listening_event & event) {
    return;
  }
  listening_event |= event;
  updateToReactor();
}

void FdEvent::addListenEvents(IOEvent event) {
  if (listening_event & event) {
    LDebug("FdEvent::addListenEvents fd[{}] already listing this event[{}]", fd,
           eventToString(event));
    return;
  }
  listening_event |= event;
  updateToReactor();
  LDebug("FdEvent::addListenEvents fd[{}] listen event[{}]", fd,
         eventToString(event));
}

void FdEvent::delListenEvents(IOEvent event) {
  if (listening_event & event) [[likely]] {
    LDebug("FdEvent::delListenEvents: fd[{}] delete event[{}]", fd,
           eventToString(event));
    listening_event &= ~event;
    updateToReactor();
    return;
  }
  LDebug("FdEvent::delListenEvents: fd[{}] don't listen event[{}], skip!!!", fd,
         eventToString(event));
}

void FdEvent::delAllListenEvents() {
  listening_event = 0;
  updateToReactor();
}

void FdEvent::updateToReactor() {
  epoll_event evnet{.events = listening_event, .data = this};
  assert(reactor != nullptr);
  reactor->addEventInThisThread(fd, evnet);
}

void FdEvent::unRegisterFromReactor() {
  reactor->delEventInThisThread(fd);
  listening_event = 0;
  read_callback = nullptr;
  write_callback = nullptr;
  close_callback = nullptr;
  error_callback = nullptr;
}

void FdEvent::setFd(int _fd) { fd = _fd; }

void FdEvent::setNonblock() {
  assert(fd != -1);
  int flag = ::fcntl(fd, F_GETFL, 0);
  if (flag & O_NONBLOCK) {
    LDebug("fd[{}] already o_nonblock", fd);
    return;
  }
  ::fcntl(fd, F_SETFL, flag | O_NONBLOCK);
  flag = ::fcntl(fd, F_GETFL, 0);
  if (flag & O_NONBLOCK) {
    LDebug("fd[{}] set o_nonblock success", fd);
  } else {
    LDebug("fd[{}] set o_nonblock error", fd);
  }
}

bool FdEvent::isNonblock() {
  assert(fd != -1);
  int flag = ::fcntl(fd, F_GETFL, 0);
  return (flag & O_NONBLOCK);
}

}; // namespace TinyNet