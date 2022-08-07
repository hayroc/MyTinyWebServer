#pragma once

#include <queue>
#include <mutex>
#include <memory>
#include <string>
#include <thread>
#include <cstdint>
#include <condition_variable>
#include <vector>

#include "../base/singleton.h"

namespace hayroc {

class Message {
public:
  void append(const std::string& message);
  int32_t avail();
  std::string get();
  bool isempty();

public:
  Message(int32_t size = 1024) : m_buffersize(size) {}

private:
  int32_t m_buffersize;
  std::queue<std::string> m_messages;
};

class Log {
friend class Singleton<Log>;

using MessagePtr = std::unique_ptr<Message>;

public:
  enum class Level : int {
    UNKNOW = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
  };

public:
  void append(const std::string& message);
  Level getLevel() const { return m_level; }
  std::string getLevelStr() const;
  void setLevel(Level level) { m_level = level; }

private:
  void run();
  void do_write(const std::string& message);

private:
  Log();
  ~Log();

private:
  int32_t m_fd;
  std::string m_dir;
  std::string m_name;
  std::string m_path;

  Level m_level;
  bool m_stop;
  int32_t m_day;
  int32_t m_logcnt;
  int32_t m_maxline;
  int32_t m_curline;

  MessagePtr m_currbuffer;
  MessagePtr m_nextbuffer;
  std::vector<MessagePtr> m_buffers;
  std::mutex m_mutex;
  std::condition_variable m_cond;
  std::thread m_thread;
};

#define LOG(level, levelstr, message)\
  if(Singleton<Log>::GetInstance()->getLevel() <= level)\
    Singleton<Log>::GetInstance()->append(levelstr + std::string(__FILE__) + "@" + std::string(__func__) + ":" + std::to_string(__LINE__) + " " + message)

#define UNKNOW(message) LOG(Log::Level::UNKNOW, "UNKNOW ", message)
#define DEBUG(message)  LOG(Log::Level::DEBUG, "DEBUG ", message)
#define INFO(message)   LOG(Log::Level::INFO, "INFO ", message)
#define WARN(message)   LOG(Log::Level::WARN, "WARN ", message)
#define ERROR(message)  LOG(Log::Level::ERROR, "ERROR ", message)
#define FATAL(message)  LOG(Log::Level::FATAL, "FATAL ", message)

}
