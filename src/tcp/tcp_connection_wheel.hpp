#pragma once

#include "tcp_connection.hpp"
#include <queue>
#include <slot.hpp>
#include <timer.hpp>
#include <vector>

namespace TinyNet {

class Reactor;

class TcpTimeWheel {
public:
  explicit TcpTimeWheel(Reactor *r, int bucket_count, int interval = 10);
  ~TcpTimeWheel();
  TcpTimeWheel(const TcpTimeWheel &) = delete;
  TcpTimeWheel &operator=(const TcpTimeWheel &) = delete;

  TcpTimeWheel(TcpTimeWheel &&r)
      : m_reactor{r.m_reactor}, m_bucket_count{r.m_bucket_count},
        m_interval{r.m_interval}, m_event{std::move(r.m_event)},
        m_wheel{std::move(r.m_wheel)} {
    m_event->task = [this] { this->onTimeCallback(); };
    r.m_reactor = nullptr;
  }

  TcpTimeWheel &operator=(TcpTimeWheel &&r) {
    m_reactor = r.m_reactor;
    m_bucket_count = r.m_bucket_count;
    m_interval = r.m_interval;
    m_event = std::move(r.m_event);
    m_event->task = [this]() { this->onTimeCallback(); };
    m_wheel = std::move(r.m_wheel);
    return *this;
  }

  void fresh(const std::shared_ptr<Slot<TcpConnection>> &);
  void setOnTimeCallback(std::function<void()>);
  void onTimeCallback();

private:
  Reactor *m_reactor{nullptr};
  int m_bucket_count{0};
  int m_interval{0};
  std::shared_ptr<TimerEvent> m_event;
  std::queue<std::vector<std::shared_ptr<Slot<TcpConnection>>>> m_wheel;
};

}; // namespace TinyNet