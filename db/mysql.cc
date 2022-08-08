#include "mysql.h"

#include "cassert"
#include "../log/log.h"
#include <cassert>
#include <memory>
#include <iostream>

namespace hayroc {

MysqlConnectionPool::MysqlConnectionPool() {

}

MysqlConnectionPool::~MysqlConnectionPool() {
  while(!m_queue.empty()) {
    MYSQL* sql = m_queue.front();
    m_queue.pop();
    mysql_close(sql);
  }
  mysql_library_end();
}

void MysqlConnectionPool::init(std::string host, int port, std::string user, std::string pass, std::string dbName, int maxconn) {
  for(int i = 0; i < maxconn; i++) {
    MYSQL* sql = nullptr;
    sql = mysql_init(sql);
    if(!sql) {
      ERROR("Mysql init error");
      assert(sql);
    }
    sql = mysql_real_connect(sql, host.data(), user.data(), pass.data(), dbName.data(), port, nullptr, 0);
    if(!sql) {
      ERROR("Mysql connect error");
      assert(sql);
    }
    m_queue.push(sql);
  }
  m_maxConnection = maxconn;
}


MYSQL* MysqlConnectionPool::getConnection() {
  std::unique_lock<std::mutex> lock(m_mutex);
  while(m_queue.empty()) {
    m_cond.wait(lock);
  }
  MYSQL* sql = m_queue.front();
  m_queue.pop();
  return sql;
}

void MysqlConnectionPool::delConnection(MYSQL* sql) {
  std::unique_lock<std::mutex> lock(m_mutex);
  m_queue.push(sql);
  m_cond.notify_one();
}

int MysqlConnectionPool::getFreeConnection() {
  std::unique_lock<std::mutex> lock(m_mutex);
  return m_queue.size();
}

}
