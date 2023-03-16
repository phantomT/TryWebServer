#include "server/web_server.h"
#include "config/config.h"

int main(int argc, char *argv[]) {
    Config config;
    config.ParseArg(argc, argv);

    const int dbPort = 3306;
    const char *dbUser = "tbb";
    const char *dbPassWd = "123456";
    const char *dbName = "webDB";

    WebServer server(
            config.port, config.trigMode, config.timeoutMs, config.optLinger,   // 端口 ET模式 timeoutMs 优雅退出
            dbPort, dbUser, dbPassWd, dbName,                       // Mysql配置
            config.connPoolNum, config.threadNum,                                       // 连接池数量 线程池数量
            config.openLog, config.logLevel, config.logQueSize);                        // 日志开关 日志等级 日志异步队列容量
    server.Start();
} 
  