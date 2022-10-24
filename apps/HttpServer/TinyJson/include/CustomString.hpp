#include <cstring>
#include <memory>
#include <type_traits>

struct CustomString {
private:
  inline static size_t InitStringLen = 2;

public:
  CustomString() : _alloc{}, _ptr{nullptr}, _len{}, _capacity{} {}
  ~CustomString() {
    if (_ptr)
      _alloc.deallocate(_ptr, _capacity);
  }
  auto getStr() { return std::string_view(_ptr, _len); }
  size_t size() const { return _len; }
  size_t capacity() const { return _capacity; }
  CustomString(const CustomString &) = delete;
  CustomString(CustomString &&rhs)
      : _alloc{rhs._alloc}, _ptr{rhs._ptr}, _len{rhs._len}, _capacity{
                                                                rhs._capacity} {
    rhs._ptr = nullptr;
    rhs._len = 0;
    rhs._capacity = 0;
  }
  CustomString &operator=(const CustomString &) = delete;
  CustomString &operator=(CustomString &&rhs) {
    CustomString tmp;
    _M_swap_data(rhs);
    tmp._M_swap_data(rhs);
    std::__alloc_on_move(_alloc, rhs._alloc);
    return *this;
  }
  // 插入一个字节
  CustomString &operator+=(char ch) {
    size_t prev = _capacity;
    if (_len >= _capacity) {
      if (_capacity == 0)
        _capacity = InitStringLen;
      while (_len >= _capacity) {
        _capacity += _capacity / 2;
      }
      char *tmp = _alloc.allocate(_capacity);
      if (_ptr) {
        std::uninitialized_copy_n(_ptr, _len, tmp);
        _alloc.deallocate(_ptr, prev);
      }
      _ptr = tmp;
    }
    _ptr[_len++] = ch;
    return *this;
  }

private:
  [[no_unique_address]] std::allocator<char> _alloc;
  char *_ptr;
  size_t _len;
  size_t _capacity;
  // copy from stl_vector
  // Do not use std::swap(_M_start, __x._M_start), etc as it loses
  // information used by TBAA.
  void _M_copy_data(CustomString const &__x) noexcept {
    _ptr = __x._ptr;
    _len = __x._len;
    _capacity = __x._capacity;
  }
  void _M_swap_data(CustomString &__x) noexcept {
    CustomString __tmp;
    __tmp._M_copy_data(*this);
    _M_copy_data(__x);
    __x._M_copy_data(__tmp);
  }
};