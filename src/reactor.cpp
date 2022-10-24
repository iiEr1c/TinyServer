#include "reactor.hpp"
#include "FdEvent.hpp"
#include <algorithm>
#include <cstring>
#include <logger.hpp>
#include <memory>
#include <sys/eventfd.h>

#include <signal.h>
#include <sys/signalfd.h>

namespace TinyNet {
Reactor::Reactor()
    : epollfd{::epoll_create1(EPOLL_CLOEXEC)},
      wakeupEvent(this, ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)), timer(this),
      pending_tasks_cross_thread(4096) {
  if (epollfd.getFd() < 0) [[unlikely]] {
    LError("Reactor::Reactor(): epollfd < 0");
    std::abort();
  }

  wakeupEvent.setReadCallback([this] { this->wakeupHandleRead(); });
  wakeupEvent.addListenEvents(FdEvent::IOEvent::READ);
  LDebug("Reactor() init: epollFd[{}], wakeupFd[{}], timerFd[{}]",
         epollfd.getFd(), wakeupEvent.getFd(), timer.getFd());
}

Reactor::~Reactor() {
  wakeupEvent.unRegisterFromReactor();
  ::close(wakeupEvent.getFd());
}

void Reactor::addEventInThisThread(int fd, epoll_event event) {
  int op = EPOLL_CTL_ADD;
  bool is_add = true;

  if (auto it = std::find(activeFd.begin(), activeFd.end(), fd);
      it != activeFd.end()) {
    is_add = false;
    op = EPOLL_CTL_MOD;
  }
  if ((::epoll_ctl(epollfd.getFd(), op, fd, &event)) != 0) {
    LError("Reactor::addEventInThisThread(): epoll_ctl "
           "error, fd[{}], is_add[{}], sys errinfo [{}]",
           fd, is_add ? "True" : "false", strerror(errno));
    return;
  }
  if (is_add) {
    activeFd.emplace_back(fd);
  }
  LDebug("Reactor::addEventInThisThread: epoll_ctl add fd[{}] success.", fd);
}

void Reactor::delEventInThisThread(int fd) {
  auto it = std::find(activeFd.begin(), activeFd.end(), fd);
  if (it == activeFd.end()) {
    LError("Reactor::delEventInThisThread: fd[{}] not in this reactor", fd);
    return;
  }
  if ((::epoll_ctl(epollfd.getFd(), EPOLL_CTL_DEL, fd, nullptr)) != 0) {
    LError("Reactor::delEventInThisThread: epoll_ctl error, fd[{}]", fd);
  }
  activeFd.erase(it);
  LDebug("Reactor::delEventInThisThread: epoll_ctl del fd[{}] success.", fd);
}

void Reactor::loop() {
  while (!isquit.test(std::memory_order_relaxed)) {

    size_t taskNums = pending_tasks_cross_thread.sizeGuess();

    for (size_t i = 0; i < taskNums; ++i) {
      std::function<void()> func;
      pending_tasks_cross_thread.read(func);
      func();
    }

    epoll_event events[MAX_RECV_EVENTS];
    int recv_event_size =
        ::epoll_wait(epollfd.getFd(), events, MAX_RECV_EVENTS, EPOLL_TIMEOUT);
    if (recv_event_size < 0) [[unlikely]] {
      if (errno == EINTR) [[likely]] {
        continue;
      } else {
        LError("Reactor::loop(): epoll_wait error, skip, errno = {}",
               strerror(errno));
      }
    } else {
      LDebug("Reactor::loop(): epoll_wait receive [{}] events",
             recv_event_size);
      for (int i = 0; i < recv_event_size; ++i) {
        auto event = events[i];
        FdEvent *ptr = static_cast<FdEvent *>(event.data.ptr);
        if (ptr != nullptr) {
          uint32_t ev = event.events;
          if (ev & EPOLLRDHUP) {
            ptr->getCloseCallback()();
          } else if (ev & (EPOLLIN | EPOLLPRI)) {
            ptr->getReadCallback()();
          } else if (ev & EPOLLOUT) {
            ptr->getWriteCallback()();
          } else {
            ptr->getErrorCallback()();
          }
        }
      }
    }

    for (const auto &func : pending_tasks_in_thread) {
      func();
    }
    pending_tasks_in_thread.clear();
  }
}

void Reactor::stop() {
  isquit.test_and_set();
  wakeup();
}

bool Reactor::addTaskCrossThread(std::function<void()> task) {

  if (pending_tasks_cross_thread.write(std::move(task))) [[likely]] {

    wakeup();
    return true;
  } else {
    return false;
  }
}

void Reactor::addTaskInThread(std::vector<std::function<void()>> tasks,
                              bool is_wakeup) {
  if (tasks.size() == 0) [[unlikely]] {
    return;
  }
  pending_tasks_in_thread.insert(pending_tasks_in_thread.end(), tasks.begin(),
                                 tasks.end());
}

void Reactor::addTaskInThread(std::function<void()> task) {
  pending_tasks_in_thread.emplace_back(std::move(task));
}

Timer &Reactor::getTimer() { return timer; }

void Reactor::wakeup() {
  uint64_t bytes = 1;
  size_t n = ::write(wakeupEvent.getFd(), &bytes, sizeof(bytes));
  if (n != sizeof(bytes)) [[unlikely]] {
    LError("Reactor::wakeup() writes [{}] bytes instead of 8, error[{}]", n,
           std::strerror(errno));
  }
}

void Reactor::wakeupHandleRead() {
  uint64_t bytes;
  size_t n = ::read(wakeupEvent.getFd(), &bytes, sizeof(bytes));
  if (n != sizeof(bytes)) [[unlikely]] {
    LError(
        "Reactor::wakeupHandleRead() read [{}] bytes instead of 8, error[{}]",
        n, std::strerror(errno));
  }
}

void Reactor::runEvery(int64_t timeMs, std::function<void()> cb) {
  auto ev = std::make_shared<TimerEvent>(timeMs, true, std::move(cb));
  timer.addTimerEvent(ev);
}
}; // namespace TinyNet