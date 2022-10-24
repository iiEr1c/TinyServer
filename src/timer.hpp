#pragma once
#include "FdEvent.hpp"
#include <functional>
#include <logger.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <sys/time.h>
#include <time.h>

namespace TinyNet {
class Reactor;

int64_t getNowMs();

struct TimerEvent {

  explicit TimerEvent(int64_t _interval, bool _is_repeated,
                      std::function<void()> _task)
      : interval{_interval}, task{_task}, is_repeated{_is_repeated} {
    timeout_timepoint = getNowMs() + _interval;
    LDebug("TimerEvent(): time event will occur at timepoint: [{}], after "
           "[{}]s, repeated:[{}]",
           timeout_timepoint, _interval / 1000,
           _is_repeated ? "true" : "false");
  }
  int64_t timeout_timepoint;
  int64_t interval;
  std::function<void()> task;
  bool is_repeated{false};
  bool is_cancel{false};
};

struct Timer : public TinyNet::FdEvent {
  Timer(Reactor *);
  ~Timer();
  void addTimerEvent(std::shared_ptr<TimerEvent> event, bool need_reset = true);
  void delTimerEvent(std::shared_ptr<TimerEvent> event);
  void resetArriveTime();

  void onTimeCallback();

private:
  std::multimap<int64_t, std::shared_ptr<TimerEvent>> pending_TimerEvents;
};
}; // namespace TinyNet