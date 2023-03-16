#include "server/web_server.h"

int main() {
    WebServer server(
            9006, 3, 60000, false,      // 端口 ET模式 timeoutMs 优雅退出
            3306, "tbb", "123456", "webDB", // Mysql配置
            12, 6,                  // 连接池数量 线程池数量
            true, 1, 1024);     // 日志开关 日志等级 日志异步队列容量
    server.Start();
} 
  