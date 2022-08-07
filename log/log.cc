#include "log.h"
#include <fcntl.h>
#include <iostream>
#include <pthread.h>
#include <thread>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <chrono>
#include <ctime>
#include <mutex>
#include <string>
#include <vector>
#include <algorithm>

namespace hayroc {

void Message::append(const std::string &message) {
  if(m_messages.size() >= m_buffersize) {
    return ;
  }
  m_messages.push(message);
}

bool Message::isempty() {
  return m_messages.empty();
}

int32_t Message::avail() {
  return m_buffersize = m_messages.size();
}
std::string Message::get() {
  std::string res = m_messages.front();
  m_messages.pop();
  return res;
}

void Log::append(const std::string &message) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if(m_currbuffer->avail() > 1) {
    m_currbuffer->append(message);
  } else {
    m_buffers.emplace_back(m_currbuffer.release());
    if(m_nextbuffer) {
      m_currbuffer = std::move(m_nextbuffer);
    } else {
      m_currbuffer.reset(new Message);
    }

    m_currbuffer->append(message);
    m_cond.notify_one();
  }
}

std::string Log::getLevelStr() const {
  switch (m_level) {
    case Level::UNKNOW: return "UNKNOW";
    case Level::DEBUG: return "DEBUG";
    case Level::INFO: return "INFO";
    case Level::WARN: return "WARN";
    case Level::ERROR: return "ERROR";
    case Level::FATAL: return "FATAL";
    default: return "?????";
  }
}

void Log::run() {
  MessagePtr newBuffer1(new Message);
  MessagePtr newBuffer2(new Message);
  std::vector<MessagePtr> bufferToWrite;

  while(!m_stop) {
    if(true) {
      std::unique_lock<std::mutex> lock(m_mutex);
      if(m_buffers.empty()) {
        m_cond.wait_for(lock, std::chrono::duration<int>(3));
      }
      m_buffers.emplace_back(m_currbuffer.release());
      m_currbuffer = std::move(newBuffer1);
      bufferToWrite.swap(m_buffers);
      if(!m_nextbuffer) {
        m_nextbuffer = std::move(newBuffer2);
      }
    }

    for(auto& p : bufferToWrite) {
      while(!p->isempty()) {
        do_write(p->get());
      }

      if(!newBuffer1) {
        newBuffer1.reset(p.release());
      } else if(!newBuffer2) {
        newBuffer2.reset(p.release());
      }
    }
    bufferToWrite.clear();
  }
}

void Log::do_write(const std::string& message) {
  m_curline++;
  std::string mess = message + "\r\n";
  time_t now = time(nullptr);
  tm* t = localtime(&now);

  bool change = false;
  if(m_day != t->tm_mday) {
    m_logcnt = 1;
    m_day = t->tm_mday;
    change = true;
  } else if(m_curline >= m_maxline) {
    m_logcnt++;
    m_curline = 1;
    change = true;
  }
  if(change) {
    m_name = std::to_string(t->tm_mon + 1) + "-" + std::to_string(t->tm_mday) + "-" + std::to_string(m_logcnt) + ".log";
    m_path = m_dir + "/" + m_name;
    close(m_fd);
    m_fd = open(m_path.c_str(), O_CREAT | O_RDWR, 0777);
  }

  std::string time = std::to_string(t->tm_mon + 1) + "/" + std::to_string(t->tm_mday) + "/" 
                     + std::to_string(t->tm_hour) + ":" + std::to_string(t->tm_min) + ":" + std::to_string(t->tm_sec);
  std::string sum = time + " " + mess;
  int ret = write(m_fd, sum.c_str(), sum.size());
  assert(ret != -1);
}

Log::Log() {
  time_t now = time(nullptr);
  tm* t = localtime(&now);

  m_dir = ".";
  m_name = std::to_string(t->tm_mon + 1) + "-" + std::to_string(t->tm_mday) + "-1.log";
  m_path = m_dir + "/" + m_name;
  m_fd = open(m_path.c_str(), O_CREAT | O_RDWR, 0777);

  m_level = Level::DEBUG;
  m_stop = false;
  m_day = t->tm_mday;
  m_logcnt = 1;
  m_maxline = 5000;
  m_curline = 1;

  m_currbuffer.reset(new Message);
  m_nextbuffer.reset(new Message);

  m_thread = std::thread([]() {
    Singleton<Log>::GetInstance()->run();
  });
  m_thread.detach();
}

Log::~Log() {
  m_stop = true;
  if(m_fd) {
    close(m_fd);
  }
}

}

