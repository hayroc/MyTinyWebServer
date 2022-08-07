#pragma once

#include <mysql/mysql.h>
#include "sql_connection_pool.h"

class connectionRAII{
public:
  connectionRAII(MYSQL **con, connection_pool *connPool);
  ~connectionRAII();

private:
  MYSQL *conRAII;
  connection_pool *poolRAII;
};
