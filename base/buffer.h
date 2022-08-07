#pragma once

#include <cstddef>
#include <string>
#include <sys/types.h>
#include <vector>
#include <algorithm>

namespace hayroc {

class Buffer {
public:
  ssize_t readFd(int fd, int& errorNum);
  ssize_t writeFd(int fd, int& errorNum);
  const char* findCRLF() const;
  const char* findCRLF(const char* start) const;
  void retrieve(size_t len);
  void retrieveAll();
  void append(const char* str, size_t len);
  void append(const std::string& str);
  void append(const Buffer& buffer);

  void swap(Buffer& lhs) {
    m_buffer.swap(lhs.m_buffer);
    std::swap(m_readIdx, lhs.m_readIdx);
    std::swap(m_writeIdx, lhs.m_writeIdx);
  }

  ssize_t readableBytes() const { return m_writeIdx - m_readIdx; }
  ssize_t writeableBytes() const { return m_buffer.size() - m_writeIdx; }
  ssize_t prependableBytes() const { return m_readIdx; }
  const char* peek() const { return begin() + m_readIdx; }
  char* beginWrite() { return begin() + m_writeIdx; }
  const char* beginWrite() const { return begin() + m_writeIdx; }
  void hasWrite(size_t len) { m_writeIdx += len; }


private:
  char* begin() {
    return &*m_buffer.begin();
  }
  const char* begin() const {
    return &*m_buffer.begin();
  }
  void makeSpace(size_t len) {
    if(writeableBytes() + prependableBytes() < len + CHEAP_PERPEND) {
      m_buffer.resize(len + m_writeIdx);
    } else {
      size_t readable = readableBytes();
      std::copy(begin() + m_readIdx, begin() + m_writeIdx, begin() + CHEAP_PERPEND);
      m_readIdx = CHEAP_PERPEND;
      m_writeIdx = CHEAP_PERPEND + readableBytes();
    }
  }

public:
  Buffer() : m_buffer(CHEAP_PERPEND + INIT_SIZE), m_readIdx(CHEAP_PERPEND), m_writeIdx(CHEAP_PERPEND) {}

public:
  static const size_t CHEAP_PERPEND = 8;
  static const size_t INIT_SIZE = 1024;

private:
  std::vector<char> m_buffer;
  size_t m_readIdx;
  size_t m_writeIdx;
};

}
