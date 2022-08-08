# MyTinyWebServer
参考游双老师的《Linux高性能服务器编程》和[TinyWebServer](https://github.com/qinguoyi/TinyWebServer)实现

## 功能
- 利用IO复用技术Epoll与线程池实现多线程的Reactor高并发模型
- 利用状态机解析HTTP请求报文，实现处理静态资源的请求
- 利用std::vector封装成自动增长的应用层缓冲区
- 利用基于std::priority_queue的时间堆，关闭超时连接
- 利用基于std::queue的阻塞队列实现异步日志

## 运行
服务器环境
- Manjaro Linux
- MySQL 8.0

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

启动
```bash
mkdir build && cd build
cmake ..
make
cd ..
./bin/webserver
```
