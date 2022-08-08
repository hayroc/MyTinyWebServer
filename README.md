# MyTinyWebServer
参考游双老师的《Linux高性能服务器编程》和[TinyWebServer](https://github.com/qinguoyi/TinyWebServer)实现

创建数据库
```sql
-- 创建MyTinyWebServer库
create database MyTinyWebServer;

-- 创建user表
USE MyTinyWebServer;
CREATE TABLE user(
  username char(50) NOT NULL,
  password char(50) NOT NULL,
  PRIMARY KEY(username)
) ENGINE=InnoDB;

-- 添加数据
INSERT INTO user(username, password) VALUES('alice', 'alice');
```
