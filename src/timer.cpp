#include "timer.hpp"
#include "reactor.hpp"
#include <cstring>
#include <iostream>
#include <logger.hpp>
#include <sys/timerfd.h>

namespace TinyNet {

int64_t getNowMs() {
  timeval val{};
  ::gettimeofday(&val, nullptr);
  int64_t ret = val.tv_sec * 1000 + val.tv_usec / 1000;
  return ret;
}

Timer::Timer(Reactor *r)
    : FdEvent(r,
              ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)) {
  setReadCallback([this]() { this->onTimeCallback(); });
  addListenEvents(IOEvent::READ);
}

Timer::~Timer() {
  unRegisterFromReactor();
  ::close(fd);
}

// todo: type => const std::shared_ptr<TimerEvent>&
void Timer::addTimerEvent(std::shared_ptr<TimerEvent> event, bool need_reset) {
  bool is_reset = false;

  if (pending_TimerEvents.empty()) {
    is_reset = true;
  } else {
    auto it = pending_TimerEvents.begin();
    if (event->timeout_timepoint < it->second->timeout_timepoint) {
      is_reset = true;
    }
  }

  pending_TimerEvents.emplace(event->timeout_timepoint, event);
  if (is_reset && need_reset) {
    resetArriveTime();
  }
}

void Timer::delTimerEvent(std::shared_ptr<TimerEvent> event) {
  event->is_cancel = true;
}

void Timer::resetArriveTime() {
  if (pending_TimerEvents.size() == 0) {
    LDebug("Timer::resetArriveTime: no timer event");
    return;
  }

  auto now = getNowMs();
  auto it = pending_TimerEvents.begin();
  if (it->first < now) {
    LDebug("Timer::resetArriveTime: the first event has already expire");
    return;
  }

  int64_t interval = it->first - now;
  itimerspec new_val{.it_interval = {.tv_sec = 0, .tv_nsec = 0},
                     .it_value = {.tv_sec = interval / 1'000,
                                  .tv_nsec = (interval % 1'000) * 1'000'000}};
  int ret = ::timerfd_settime(fd, 0, &new_val, nullptr);
  if (ret != 0) [[unlikely]] {
    LError("Timer::resetArriveTime: timer_settime error, interval = {}",
           interval);
  }
}

void Timer::onTimeCallback() {

  uint64_t bytes;
  size_t n = ::read(fd, &bytes, sizeof(bytes));
  if (n != sizeof(bytes)) [[unlikely]] {
    LWarn("Timer::onTimeCallback read {} bytes instead of 8", n);
  }

  auto now = getNowMs();
  std::vector<std::shared_ptr<TimerEvent>> tmps;
  std::vector<std::function<void()>> tasks;
  auto it = pending_TimerEvents.begin();
  for (; it != pending_TimerEvents.end(); ++it) {
    if (it->first <= now) {
      if (!it->second->is_cancel) {
        tmps.push_back(it->second);
        tasks.push_back(it->second->task);
      }
    } else {
      break;
    }
  }

  reactor->addTaskInThread(tasks);

  pending_TimerEvents.erase(pending_TimerEvents.begin(), it);
  for (const auto &timeEvent : tmps) {
    if (timeEvent->is_repeated) {
      timeEvent->timeout_timepoint = getNowMs() + timeEvent->interval;
      addTimerEvent(timeEvent, false);
    }
  }

  resetArriveTime();
}

}; // namespace TinyNet