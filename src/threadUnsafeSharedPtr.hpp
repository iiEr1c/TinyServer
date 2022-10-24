#pragma once
#include <cstddef>
#include <functional>

namespace TinyNet {
template <typename T> class ThreadUnsafeSharedPtr;
template <typename T> class ThreadUnsafeWeakPtr;
template <typename Tp> class shared_weak_count;

template <typename Tp> class shared_weak_count {
public:
  constexpr shared_weak_count() noexcept
      : m_use_count{0}, m_weak_count{0}, m_ptr{nullptr}, m_deleter{} {}

  template <typename Yp>
  explicit shared_weak_count(Yp *ptr, std::function<void(Yp *)> deleter)
      : m_use_count{1}, m_weak_count{1}, m_ptr{ptr}, m_deleter{
                                                         std::move(deleter)} {}

  inline void add_ref_copy() { m_use_count += 1; }

  inline void add_weak_copy() { m_weak_count += 1; }

  inline int get_use_count() { return m_use_count; }

  inline void dispose() {
    if (m_deleter) [[likely]] {
      m_deleter(m_ptr);
    } else {
      delete m_ptr;
    }
  }

  inline void destroy() { delete this; }

  void release() {
    if (--m_use_count == 0) {
      dispose();
      if (--m_weak_count == 0) {
        destroy();
      }
    }
  }

  void weak_release() {
    if (--m_weak_count == 0) {
      destroy();
    }
  }

private:
  // friend class ThreadUnsafeSharedPtr<Tp>;
  // friend class ThreadUnsafeWeakPtr<Tp>;
  int m_use_count;
  int m_weak_count;
  Tp *m_ptr;
  std::function<void(Tp *)> m_deleter;
};

// todo: operator->和operator *
template <typename T> class ThreadUnsafeSharedPtr {

public:
  constexpr ThreadUnsafeSharedPtr() : m_ptr{nullptr}, m_refcount{nullptr} {}

  constexpr ThreadUnsafeSharedPtr(nullptr_t) noexcept
      : ThreadUnsafeSharedPtr() {}

  template <typename Yp>
  ThreadUnsafeSharedPtr(Yp *ptr, std::function<void(Yp *)> deleter)
      : m_ptr{ptr}, m_refcount{
                        new shared_weak_count<Yp>(ptr, std::move(deleter))} {}

  ThreadUnsafeSharedPtr(const ThreadUnsafeWeakPtr<T> &r) noexcept
      : m_refcount{r.m_weakcount} {
    if (m_refcount == nullptr) [[unlikely]] {
      m_ptr = nullptr;
    } else {
      bool lockSucc = m_refcount->get_use_count() > 0;
      if (lockSucc) [[likely]] {
        m_ptr = r.m_ptr;
        m_refcount->add_ref_copy();
      } else {
        m_ptr = nullptr;
        m_refcount = nullptr;
      }
    }
  }

  ThreadUnsafeSharedPtr(const ThreadUnsafeSharedPtr &r)
      : m_ptr{r.m_ptr}, m_refcount{r.m_refcount} {
    if (m_refcount != nullptr) {
      m_refcount->add_ref_copy();
    }
  }

  ThreadUnsafeSharedPtr(ThreadUnsafeSharedPtr &&r)
      : m_ptr{r.m_ptr}, m_refcount{r.m_refcount} {
    r.m_ptr = nullptr;
    r.m_refcount = nullptr;
  }

  ThreadUnsafeSharedPtr &operator=(const ThreadUnsafeSharedPtr &r) {
    m_ptr = r.m_ptr;
    shared_weak_count<T> *tmp = r.m_refcount;
    if (tmp != m_refcount) {
      if (tmp != nullptr) {
        tmp->add_ref_copy();
      }

      if (m_refcount != nullptr) {
        m_refcount->release();
      }

      m_refcount = tmp;
    }
    return *this;
  }

  ThreadUnsafeSharedPtr &operator=(ThreadUnsafeSharedPtr &&r) {
    m_ptr = r.m_ptr;
    // 如果this对象不为nullptr, 则release
    if (m_refcount != nullptr) {
      m_refcount->release();
    }
    m_refcount = r.m_refcount;
    r.m_ptr = nullptr;
    r.m_refcount = nullptr;
    return *this;
  }

  T *operator->() const noexcept { return m_ptr; }

  T *get() const noexcept { return m_ptr; }

  T &operator*() const noexcept { return *m_ptr; }

  int use_count() const noexcept {
    return m_refcount ? m_refcount->get_use_count() : 0;
  }

  explicit operator bool() const noexcept { return m_ptr != nullptr; }

  ~ThreadUnsafeSharedPtr() {
    if (m_refcount != nullptr) {
      m_refcount->release();
    }
  }

private:
  friend class ThreadUnsafeWeakPtr<T>;
  T *m_ptr;
  shared_weak_count<T> *m_refcount;
};

template <typename T> class ThreadUnsafeWeakPtr {
public:
  constexpr ThreadUnsafeWeakPtr() : m_ptr{nullptr}, m_weakcount{nullptr} {}

  constexpr ThreadUnsafeWeakPtr(nullptr_t) noexcept : ThreadUnsafeWeakPtr() {}

  ThreadUnsafeWeakPtr(const ThreadUnsafeSharedPtr<T> &r) noexcept
      : m_ptr{r.m_ptr}, m_weakcount{r.m_refcount} {
    if (m_weakcount != nullptr) {
      m_weakcount->add_weak_copy();
    }
  }

  ThreadUnsafeWeakPtr(const ThreadUnsafeWeakPtr &r)
      : m_ptr{r.m_ptr}, m_weakcount{r.m_weakcount} {
    if (m_weakcount != nullptr) {
      m_weakcount->add_weak_copy();
    }
  }

  ThreadUnsafeWeakPtr(ThreadUnsafeWeakPtr &&r) noexcept
      : m_ptr{r.m_ptr}, m_weakcount{r.m_weakcount} {
    r.m_ptr = nullptr;
    r.m_weakcount = nullptr;
  }

  ThreadUnsafeWeakPtr &operator=(const ThreadUnsafeWeakPtr &r) noexcept {
    m_ptr = r.m_ptr;
    shared_weak_count<T> *tmp = r.m_weakcount;
    // 如果r对象不为null, 则增加其weak_count
    if (tmp != nullptr) {
      tmp->add_weak_copy();
    }

    // 如果this对象不为null, 则需要减少其weak_count
    if (m_weakcount != nullptr) {
      m_weakcount->weak_release();
    }
    m_weakcount = tmp;
    return *this;
  }

  ThreadUnsafeWeakPtr &operator=(ThreadUnsafeWeakPtr &&r) noexcept {
    m_ptr = r.m_ptr;
    if (m_weakcount != nullptr) {
      m_weakcount->weak_release(); // 更新this对象的weakcount状态
    }
    m_weakcount = r.m_weakcount;
    r.m_ptr = nullptr;
    r.m_weakcount = nullptr;
    return *this;
  }

  ~ThreadUnsafeWeakPtr() {
    if (m_weakcount != nullptr) {
      m_weakcount->weak_release();
    }
  }

  ThreadUnsafeSharedPtr<T> lock() const noexcept {
    return ThreadUnsafeSharedPtr<T>(*this);
  }

  int use_count() const noexcept {
    return m_weakcount != nullptr ? m_weakcount->get_use_count() : 0;
  }

private:
  friend class ThreadUnsafeSharedPtr<T>;
  T *m_ptr;
  shared_weak_count<T> *m_weakcount;
};

template <typename Tp>
inline bool operator==(const ThreadUnsafeSharedPtr<Tp> &a, nullptr_t) noexcept {
  return !a;
}
}; // namespace TinyNet