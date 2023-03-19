#include "config.h"

Config::Config() {
    Init();
}

void Config::Init() {
    std::ifstream fin("./server_config.json");
    std::stringstream ss;
    ss << fin.rdbuf();
    std::string s(ss.str());
    auto config = parser(s).value();
    auto sqlNode = config["mysql"];
    sqlPort = sqlNode["port"].getInt();
    sqlUser = sqlNode["username"].getString();
    sqlPwd = sqlNode["password"].getString();
    dbName = sqlNode["database"].getString();

    auto serverNode = config["server"];
    port = serverNode["port"].getInt();
    trigMode = serverNode["trigMode"].getInt();
    timeoutMs = serverNode["timeoutMs"].getInt();
    optLinger = serverNode["optLinger"].getBool();
    connPoolNum = serverNode["connPoolNum"].getInt();
    threadNum = serverNode["threadNum"].getInt();
    openLog = serverNode["openLog"].getBool();
    logLevel = serverNode["logLevel"].getInt();
    logQueSize = serverNode["logQueSize"].getInt();
}
