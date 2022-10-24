#pragma once
#include <functional>
#include <memory>
#include <logger.hpp>

namespace TinyNet {
template <typename T> class Slot {
public:
  Slot(std::shared_ptr<T> ptr, std::function<void(const std::shared_ptr<T> &)> cb)
      : m_weak_ptr{std::move(ptr)}, m_callback{std::move(cb)} {}

  ~Slot() {
    auto ptr = m_weak_ptr.lock();
    if (ptr) {
      LDebug("time_wheel active");
      m_callback(ptr);
    }
  }

private:
  std::weak_ptr<T> m_weak_ptr;
  std::function<void(const std::shared_ptr<T> &)> m_callback;
};
}; // namespace TinyNet