#ifndef CONFIG_H
#define CONFIG_H

#include <unistd.h>
#include <string>

class Config {
public:
    Config();

    void ParseArg(int argc, char *argv[]);

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
