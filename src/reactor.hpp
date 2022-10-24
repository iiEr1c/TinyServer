#pragma once
#include "FdEvent.hpp"
#include "epollfd.hpp"
#include "spsc.hpp"
#include "tcp/tcp_connection_wheel.hpp"
#include "timer.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

namespace TinyNet {
class Reactor {
public:
  explicit Reactor();
  ~Reactor();
  void addEventInThisThread(int fd, epoll_event event);
  void delEventInThisThread(int fd);

  bool addTaskCrossThread(std::function<void()> task);
  void addTaskInThread(std::vector<std::function<void()>> tasks,
                       bool is_wakeup = true);
  void addTaskInThread(std::function<void()>);
  Timer &getTimer();
  void wakeup();
  void wakeupHandleRead();
  void loop();
  void stop();

  void runEvery(int64_t timeMs, std::function<void()> cb);

private:
  inline static int MAX_RECV_EVENTS = 32;
  inline static int EPOLL_TIMEOUT = 10000; // ms
  EpollFd epollfd;

  std::atomic_flag isquit{false};
  std::vector<int> activeFd;

  FdEvent wakeupEvent;
  Timer timer;
  std::vector<std::function<void()>> pending_tasks_in_thread;
  spsc<std::function<void()>> pending_tasks_cross_thread;
};
}; // namespace TinyNet