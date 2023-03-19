#ifndef CONFIG_H
#define CONFIG_H

#include <unistd.h>
#include <string>

#include "json_util.h"

class Config {
public:
    Config();

    void Init();

    int sqlPort;
    std::string sqlUser;
    std::string sqlPwd;
    std::string dbName;

    int port;
    int trigMode;
    int timeoutMs;
    bool optLinger;
    int connPoolNum;
    int threadNum;
    bool openLog;
    int logLevel;
    int logQueSize;
};

#endif //CONFIG_H
