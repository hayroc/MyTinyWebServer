#include "sql_connection_pool.h"

connection_pool::connection_pool() {
  m_CurConn = 0;
  m_FreeConn = 0;
}

void connection_pool::init(string url, string User, string PassWord, string DBName, int Port, int MaxConn, int close_log) {
  m_url = url;
  m_Port = Port;
  m_User = User;
  m_PassWord = PassWord;
  m_DatabaseName = DBName;
  m_close_log = close_log;

  for (int i = 0; i < MaxConn; i++)
  {
    MYSQL *con = NULL;
    con = mysql_init(con);

    if (con == NULL)
    {
    	// LOG_ERROR("MySQL Error");
    	exit(1);
    }
    con = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(), DBName.c_str(), Port, NULL, 0);

    if (con == NULL)
    {
      LOG_ERROR("MySQL Error");
      exit(1);
    }
    connList.push_back(con);
    ++m_FreeConn;
  }

  reserve = sem(m_FreeConn);

  m_MaxConn = m_FreeConn;
}
