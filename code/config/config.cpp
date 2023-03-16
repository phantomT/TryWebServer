#include "config.h"

Config::Config() {
    port = 9006;
    trigMode = 3;
    timeoutMs = 60000;
    optLinger = false;
    connPoolNum = 12;
    threadNum = 6;
    openLog = true;
    logLevel = 1;
    logQueSize = 1024;
}

void Config::ParseArg(int argc, char **argv) {
    int opt;
    const char *str = "p:r:m:o:c:t:s:l:q:";
    while ((opt = getopt(argc, argv, str)) != -1) {
        switch (opt) {
            case 'p': {
                port = std::stoi(optarg);
                break;
            }
            case 'r': {
                trigMode = std::stoi(optarg);
                break;
            }
            case 'm': {
                timeoutMs = std::stoi(optarg);
                break;
            }
            case 'o': {
                optLinger = std::stoi(optarg);
                break;
            }
            case 'c': {
                connPoolNum = std::stoi(optarg);
                break;
            }
            case 't': {
                threadNum = std::stoi(optarg);
                break;
            }
            case 's': {
                openLog = std::stoi(optarg);
                break;
            }
            case 'l': {
                logLevel = std::stoi(optarg);
                break;
            }
            case 'q': {
                logQueSize = std::stoi(optarg);
            }
            default:
                break;
        }
    }
}
