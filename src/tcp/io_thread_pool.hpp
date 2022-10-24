#pragma once
#include <vector>

namespace TinyNet {

class IOThread;

struct IOThreadPool {
  IOThreadPool(int size);
  ~IOThreadPool();
  IOThread *getIOThread();
  IOThreadPool(const IOThreadPool &) = delete;
  IOThreadPool &operator=(const IOThreadPool &) = delete;
  IOThreadPool(IOThreadPool &&) = delete;
  IOThreadPool &operator=(IOThreadPool &&) = delete;

private:
  int m_size{0};
  int m_index{-1};

  std::vector<IOThread> m_io_threads;
};
}; // namespace TinyNet