#ifndef LOG_H
#define LOG_H

#include <cstring>
#include <cstdarg>
#include <cassert>
#include <mutex>
#include <string>
#include <thread>
#include <sys/stat.h>
#include <sys/time.h>

#include "block_queue.h"
#include "../buffer/buffer.h"

class Log {
public:
    void Init(int level, const char *path = "./log",
              const char *suffix = ".log",
              int maxQueueCapacity = 1024);

    static Log *Instance();

    static void FlushLogThread();

    void Write(int level, const char *format, ...);

    void Flush();

    int GetLevel();

    void SetLevel(int level);

    bool IsOpen() const { return l_isOpen; }

private:
    Log();

    void AppendLogLevelTitle(int level);

    virtual ~Log();

    void AsyncWrite();

private:
    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES = 50000;

    const char *l_path;
    const char *l_suffix;

    int L_MAX_LINES;

    int l_lineCnt;
    int l_toDay;

    bool l_isOpen;

    Buffer l_buff;
    int l_level;
    bool isAsync;

    FILE *l_fp;
    std::unique_ptr<BlockDeque<std::string>> l_blQue;
    std::unique_ptr<std::thread> l_writeThread;
    std::mutex l_mutex;
};

#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::Instance();\
        if (log->IsOpen() && log->GetLevel() <= level) {\
            log->Write(level, format, ##__VA_ARGS__); \
            log->Flush();\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

#endif //LOG_H