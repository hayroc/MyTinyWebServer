#pragma once

#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>

#include "../base/locker.h"
#include "../base/singleton.h"

using std::string;
using std::list;

class connection_pool {
template<typename T>
friend class Singleton;

public:
  MYSQL* GetConnection();
  bool ReleaseConnection(MYSQL* conn);
  int GetFreeConn();
  void DestroyPool();
  void init(string url, string User, string PassWord, string DataBaseName, int Port, int MaxConn, int close_log);

private:
  connection_pool();
  ~connection_pool();

private:
  int m_MaxConn;
  int m_CurConn;
  int m_FreeConn;
  locker lock;
  list<MYSQL*> connList;
  sem reserve;

public:
  string m_url;
  string m_Port;
  string m_User;
  string m_PassWord;
  string m_DatabaseName;
  int m_close_log; //日志开关
};
