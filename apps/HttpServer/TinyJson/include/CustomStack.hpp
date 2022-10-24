
#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

class CustomStack {
private:
  static inline size_t InitStackSize = 16;
  static inline size_t alignment = 8;

public:
  CustomStack(size_t size = InitStackSize)
      : _base{static_cast<char *>(std::aligned_alloc(alignment, size))},
        _size{size}, _top{} {}
  ~CustomStack() { std::free(_base); }
  size_t top() const { return _top; }
  size_t size() const { return _size; }
  CustomStack(const CustomStack &) = delete;
  CustomStack &operator=(const CustomStack &) = delete;

  template <typename T> std::pair<size_t, size_t> push(T &&val) {
    size_t padding = PaddingSize<std::decay_t<T>>();
    Shrink(padding + sizeof(std::decay_t<T>));
    ::new (BasePtr() + top() + padding) T(std::forward<T>(val));
    size_t prevTop = top();
    addTop(padding + sizeof(std::decay_t<T>));
    return {prevTop, padding};
  }

  char *pop(size_t len) {
    assert(len <= top());
    subTop(len);
    return BasePtr() + top();
  }

private:
  char *_base;
  size_t _size;
  size_t _top;
  template <typename T> size_t PaddingSize() {
    size_t curTop = reinterpret_cast<size_t>(BasePtr()) + top();
    constexpr size_t align = std::alignment_of_v<std::decay_t<T>>;
    size_t padding = (align - curTop % align) % align;
    return padding;
  }
  char *BasePtr() const { return _base; }
  void addTop(size_t len) { _top += len; }
  void subTop(size_t len) { _top -= len; }

  void Shrink(size_t len) {
    size_t prevSize = top();
    while (top() + len > size()) {
      _size += _size >> 1;
    }
    char *tmp = static_cast<char *>(std::aligned_alloc(alignment, size()));
    std::memcpy(tmp, _base, prevSize);
    std::free(_base);
    _base = tmp;
  }
};