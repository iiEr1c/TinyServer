#pragma once
#include <memory>
#include <string>
#include <vector>

namespace TinyNet {
class TcpBuffer {
public:
  inline static int initSize = 1024;
  explicit TcpBuffer(int size = initSize);
  ~TcpBuffer();
  size_t readAble();
  size_t writeAble();
  size_t readIndex() const;
  size_t writeIndex() const;

  const char *readableRawPtr() const;
  const char *writeableRawPtr() const;

  void writeToBuffer(const char *input, size_t size);
  void readFromBuffer(std::vector<char> &re, size_t size);
  void readFromBuffer(std::string &, size_t size);
  ssize_t FromFdWriteBuffer(int fd, int *savedErrno);

  void resizeBuffer(size_t size);
  void clearBuffer();
  std::vector<char> getBufferVector();
  std::string getBufferString();
  void adjustBuffer();
  size_t getSize() const;
  void retrieve(size_t);

private:
  size_t read_index{0};
  size_t write_index{0};
  std::vector<char> m_buffer;
};
}; // namespace TinyNet