#include "server/web_server.h"
#include "config/config.h"

int main() {
    Config config;

    WebServer server(
            config.port, config.trigMode, config.timeoutMs, config.optLinger,   // 端口 ET模式 timeoutMs 优雅退出
            config.sqlPort, config.sqlUser.c_str(),                               // Mysql配置
            config.sqlPwd.c_str(), config.dbName.c_str(),
            config.connPoolNum, config.threadNum,                               // 连接池数量 线程池数量
            config.openLog, config.logLevel, config.logQueSize);                // 日志开关 日志等级 日志异步队列容量
    server.Start();
} 
  