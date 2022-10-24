#include <iostream>
#include <logger.hpp>
#include <memory>
#include <reactor.hpp>
#include <tcp/tcp_connection.hpp>
#include <tcp/tcp_connection_wheel.hpp>

namespace TinyNet {

TcpTimeWheel::TcpTimeWheel(Reactor *r, int bucket_count, int interval)
    : m_reactor{r}, m_bucket_count{bucket_count},
      m_interval{interval}, m_event{} {

  for (int i = 0; i < bucket_count; ++i) {
    m_wheel.push(std::vector<std::shared_ptr<Slot<TcpConnection>>>{});
  }

  m_event = std::make_shared<TimerEvent>(interval * 1000, true,
                                         [this] { this->onTimeCallback(); });
  m_reactor->getTimer().addTimerEvent(m_event);
}

TcpTimeWheel::~TcpTimeWheel() {
  if (m_reactor != nullptr) {
    m_reactor->getTimer().delTimerEvent(m_event);
  }
}

void TcpTimeWheel::fresh(const std::shared_ptr<Slot<TcpConnection>> &slot) {
  LDebug("TcpTimeWheel::fresh(): fresh connection");
  m_wheel.back().emplace_back(slot);
}

void TcpTimeWheel::onTimeCallback() {
  LDebug("m_wheel.size() = [{}]", m_wheel.size());
  LDebug("TcpTimeWheel::onTimeCallback(): ^^^___^^^ m_wheel.size() = [{}]",
         m_wheel.size());
  m_wheel.pop();
  m_wheel.push(std::vector<std::shared_ptr<Slot<TcpConnection>>>{});
}

}; // namespace TinyNet