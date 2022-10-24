#include "io_thread_pool.hpp"
#include "io_thread.hpp"
#include <iostream>

namespace TinyNet {

IOThreadPool::IOThreadPool(int size) : m_size(size) {
  LDebug("IOThreadPool::IOThreadPool: create [{}] thread", size);
  m_io_threads.reserve(size);
  for (int i = 0; i < size; ++i) {
    m_io_threads.emplace_back(i);
    m_io_threads.back().start();
    LDebug("IOThreadPool::IOThreadPool: already create [{}] thread", i + 1);
  }
}

IOThread *IOThreadPool::getIOThread() {
  ++m_index;
  if (m_index == m_size) [[unlikely]] {
    m_index = 0;
  }
  return &m_io_threads[m_index];
}

IOThreadPool::~IOThreadPool() {
  //
  LDebug("IOThreadPool::~IOThreadPool()");
}
}; // namespace TinyNet