#include "tcp_buffer.hpp"
#include <cstring>
#include <sys/uio.h>

namespace TinyNet {

TcpBuffer::TcpBuffer(int size) { m_buffer.resize(size); }

TcpBuffer::~TcpBuffer() {}

size_t TcpBuffer::readAble() { return write_index - read_index; }

size_t TcpBuffer::writeAble() { return m_buffer.size() - write_index; }

size_t TcpBuffer::readIndex() const { return read_index; }

size_t TcpBuffer::writeIndex() const { return write_index; }

const char *TcpBuffer::readableRawPtr() const {
  return m_buffer.data() + read_index;
}

const char *TcpBuffer::writeableRawPtr() const {
  return m_buffer.data() + write_index;
}

void TcpBuffer::resizeBuffer(size_t size) {
  std::vector<char> tmp(size);
  size_t c = std::min(size, readAble());
  std::copy_n(m_buffer.begin() + readIndex(), c, tmp.begin());
  m_buffer.swap(tmp);
  read_index = 0;
  write_index = c;
}

void TcpBuffer::writeToBuffer(const char *buf, size_t size) {
  if (size >= writeAble()) {
    size_t tmp = write_index + size;
    size_t new_size = tmp + tmp / 2;
    resizeBuffer(new_size);
  }

  std::copy_n(buf, size, m_buffer.begin() + write_index);
  write_index += size;
}

void TcpBuffer::readFromBuffer(std::vector<char> &re, size_t size) {
  if (readAble() == 0) {
    return;
  }
  size_t read_size = readAble() > size ? size : readAble();
  std::vector<char> tmp(read_size);
  std::copy_n(m_buffer.begin() + read_index, read_size, tmp.begin());
  re.swap(tmp);
  read_index += read_size;
  adjustBuffer();
}

void TcpBuffer::readFromBuffer(std::string &str, size_t size) {
  size_t can_read = readAble();
  if (can_read == 0) [[unlikely]] {
    return;
  }
  size_t read_size = can_read > size ? size : can_read;
  str.append(m_buffer.begin() + read_index,
             m_buffer.begin() + read_index + read_size);
  read_index += read_size;
  adjustBuffer();
}

ssize_t TcpBuffer::FromFdWriteBuffer(int fd, int *savedErrno) {
  char extraBuf[65536];
  iovec vec[2];
  const size_t writable = writeAble();
  vec[0] = {.iov_base = m_buffer.data() + write_index, .iov_len = writable};
  vec[1] = {.iov_base = extraBuf, .iov_len = sizeof(extraBuf)};

  const int iovcnt = (writable < sizeof(extraBuf)) ? 2 : 1;
  const ssize_t n = ::readv(fd, vec, iovcnt);
  if (n < 0) [[unlikely]] {
    *savedErrno = errno;
  } else if (static_cast<size_t>(n) <= writable) {
    write_index += n;
    adjustBuffer();
  } else {
    write_index = m_buffer.size();
    writeToBuffer(extraBuf, static_cast<size_t>(n) - writable);
  }

  return n;
}

void TcpBuffer::adjustBuffer() {
  if (read_index > m_buffer.size() / 3) {
    size_t count = readAble();
    ::memmove(m_buffer.data(), m_buffer.data() + read_index, count);
    read_index = 0;
    write_index = count;
  }
}

size_t TcpBuffer::getSize() const { return m_buffer.size(); }

void TcpBuffer::clearBuffer() {
  m_buffer.clear();
  read_index = 0;
  write_index = 0;
}

void TcpBuffer::retrieve(size_t n) {
  read_index += n;
  adjustBuffer();
}

std::string TcpBuffer::getBufferString() {
  std::string ret(readAble(), 0);
  std::copy_n(m_buffer.data() + read_index, readAble(), ret.data());
  return ret;
}

inline std::vector<char> TcpBuffer::getBufferVector() { return m_buffer; }

}; // namespace TinyNet