#pragma once
#include <cassert>
#include <concepts>
#include <logger.hpp>
#include <memory>
#include <type_traits>

namespace TinyNet {
template <typename T> union StoredObj {
  T obj;
  StoredObj *next;
};

// clang-format off
template <typename T>
concept Constrain = requires(T) {
  requires sizeof(T) >= 8;
  requires std::alignment_of_v<T> >= 8;
};
// clang-format on

template <Constrain T, size_t init_num> class ObjectPool {
public:
  ObjectPool() : m_pool_blocks(m_alloc.allocate(init_num)) {
    assert(m_pool_blocks != nullptr);
    // 初始化
    init();
  }
  ~ObjectPool() {
    if (m_pool_blocks != nullptr) {
      m_alloc.deallocate(m_pool_blocks, init_num);
    }
  }

  template <typename... Args> T *acquire(Args &&...args) {
    if (m_free_list_head == nullptr) [[unlikely]] {
      return nullptr;
    } else {
      auto block{m_free_list_head};
      m_free_list_head = m_free_list_head->next;
      ::new (block) T{std::forward<Args>(args)...};
      LDebug("ObjectPool<T, size>::acquire() acquire ptr[{}] memory",
             fmt::ptr(block));
      return reinterpret_cast<T *>(block);
    }
  }

  void release(void *ptr) {
    LDebug("ObjectPool<T, size>::release() release ptr[{}] memory",
           fmt::ptr(ptr));
    assert(ptr != nullptr);
    // deconstructor
    reinterpret_cast<T *>(ptr)->~T();
    auto block{reinterpret_cast<StoredObj<T> *>(ptr)};
    block->next = m_free_list_head;
    m_free_list_head = block;
  }

private:
  void init() {
    auto prev{get_block_addr(0)};
    prev->next = nullptr;
    m_free_list_head = prev;
    for (size_t i = 1; i < init_num; ++i) {
      auto cur{get_block_addr(i)};
      prev->next = cur;
      prev = cur;
    }
    prev->next = nullptr;
  }

  auto get_block_addr(size_t index) {
    return reinterpret_cast<StoredObj<T> *>(
        std::addressof(m_pool_blocks[index]));
  }

private:
  using StorageType = typename std::aligned_storage<sizeof(T)>::type;
  [[no_unique_address]] std::allocator<StorageType> m_alloc;
  StorageType *m_pool_blocks;
  StoredObj<T> *m_free_list_head;
};
} // namespace TinyNet