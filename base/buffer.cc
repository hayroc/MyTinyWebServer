#include "buffer.h"
#include <algorithm>
#include "../log/log.h"
#include <cstddef>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

namespace hayroc {

ssize_t Buffer::readFd(int fd, int& errorNum) {
  iovec iov[2];
  ssize_t size = 0;
  ssize_t total = 0;
  char buffer[65535];

  while (true) {
    size_t writeable = writeableBytes();
    iov[0].iov_base = begin() + m_writeIdx;
    iov[0].iov_len = writeable;
    iov[1].iov_base = buffer;
    iov[1].iov_len = 65535;
    size = readv(fd, iov, 2);

    if(size <= 0) {
      break;
    }
    total += size;
    if(size < writeable) {
      hasWrite(size);
    } else {
      m_writeIdx = m_buffer.size();
      append(buffer, size - writeable);
    }
  }

  if(size == -1 && errno != EAGAIN) {
    errorNum = errno;
    return -1;
  } else {
    return total;
  }
}

ssize_t Buffer::writeFd(int fd, int& errorNum) {
  ssize_t size = 0;
  ssize_t beg = 0;
  ssize_t end = readableBytes();
  while(beg < end) {
    size = send(fd, begin() + m_readIdx, readableBytes(), MSG_NOSIGNAL);
    if(size == -1 && errno != EAGAIN) {
      break;
    }
    if(size <= 0 && errno == EAGAIN) {
      break;
    }
    m_readIdx += size;
    beg += size;
  }
  if(size == -1 && errno != EAGAIN) {
    errorNum = errno;
    return -1;
  } else {
    return beg;
  }
}

const char* Buffer::findCRLF() const {
  const char* CRLF = "\r\n";
  const char* res = std::search(peek(), beginWrite(), CRLF, CRLF + 2);
  return res == beginWrite() ? nullptr : res;
}

const char* Buffer::findCRLF(const char* start) const {
  const char* CRLF = "\r\n";
  const char* res = std::search(start, beginWrite(), CRLF, CRLF + 2);
  return res == beginWrite() ? nullptr : res;
}

void Buffer::retrieve(size_t len) {
  if(readableBytes() > len) {
    m_readIdx += len;
  } else {
    retrieveAll();
  }
}

void Buffer::retrieveAll() {
  m_readIdx = CHEAP_PERPEND;
  m_writeIdx = CHEAP_PERPEND;
}

void Buffer::append(const char* str, size_t len) {
  if(len > writeableBytes()) {
    makeSpace(len);
  }
  std::copy(str, str + len, beginWrite());
  hasWrite(len);
}

void Buffer::append(const std::string& str) {
  append(str.data(), str.length());
}

void Buffer::append(const Buffer& lhs) {
  append(lhs.peek(), lhs.readableBytes());
}

}
