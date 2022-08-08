#pragma once

#include <memory>
#include <mysql/mysql.h>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <string>
#include <iostream>

#include "../base/singleton.h"

namespace hayroc {

class MysqlConnectionPool {
public:
  MYSQL* getConnection();
  void delConnection(MYSQL* sql);
  int getFreeConnection();
  void init(std::string host, int port, std::string user, std::string pass, std::string dbName, int maxconn = 10);

public:
  MysqlConnectionPool();
  ~MysqlConnectionPool();

private:
  std::string host;
  std::string user;
  std::string pass;
  std::string dbName;
  int port;
  int maxconn;
  int m_maxConnection;

  std::queue<MYSQL*> m_queue;
  std::mutex m_mutex;
  std::condition_variable m_cond;
};

class MysqlGuard {
public:
  MysqlGuard(MYSQL** sql) {
    *sql = Singleton<MysqlConnectionPool>::GetInstance()->getConnection();
    m_sql = *sql;
  }
  ~MysqlGuard() {
    Singleton<MysqlConnectionPool>::GetInstance()->delConnection(m_sql);
    m_sql = nullptr;
  }

private:
  MYSQL* m_sql;
};

}
