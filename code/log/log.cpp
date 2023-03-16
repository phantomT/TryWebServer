#include "log.h"

Log::Log() {
    l_lineCnt = 0;
    isAsync = false;
    l_writeThread = nullptr;
    l_blQue = nullptr;
    l_toDay = 0;
    l_fp = nullptr;
}

Log::~Log() {
    if (l_writeThread && l_writeThread->joinable()) {
        while (!l_blQue->Empty()) {
            l_blQue->Flush();
        }
        l_blQue->Close();
        l_writeThread->join();
    }
    if (l_fp) {
        std::lock_guard<std::mutex> locker(l_mutex);
        Flush();
        fclose(l_fp);
    }
}

int Log::GetLevel() {
    std::lock_guard<std::mutex> locker(l_mutex);
    return l_level;
}

void Log::SetLevel(int level) {
    std::lock_guard<std::mutex> locker(l_mutex);
    l_level = level;
}

void Log::Init(int level = 1, const char *path, const char *suffix,
               int maxQueueCapacity) {
    l_isOpen = true;
    l_level = level;
    if (maxQueueCapacity > 0) {
        isAsync = true;
        if (!l_blQue) {
            std::unique_ptr<BlockDeque<std::string>> newDeque(new BlockDeque<std::string>);
            l_blQue = std::move(newDeque);

            std::unique_ptr<std::thread> NewThread(new std::thread(FlushLogThread));
            l_writeThread = std::move(NewThread);
        }
    } else {
        isAsync = false;
    }

    l_lineCnt = 0;

    time_t timer = time(nullptr);
    struct tm *sysTime = localtime(&timer);
    struct tm t = *sysTime;
    l_path = path;
    l_suffix = suffix;
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s",
             l_path, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, l_suffix);
    l_toDay = t.tm_mday;

    {
        std::lock_guard<std::mutex> locker(l_mutex);
        l_buff.RetrieveAll();
        if (l_fp) {
            Flush();
            fclose(l_fp);
        }

        l_fp = fopen(fileName, "a");
        if (l_fp == nullptr) {
            mkdir(l_path, 0777);
            l_fp = fopen(fileName, "a");
        }
        assert(l_fp != nullptr);
    }
}

void Log::Write(int level, const char *format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t tSec = now.tv_sec;
    struct tm *sysTime = localtime(&tSec);
    struct tm t = *sysTime;
    va_list vaList;

    // 日志日期 日志行数
    if (l_toDay != t.tm_mday || (l_lineCnt && (l_lineCnt % MAX_LINES == 0))) {
        std::unique_lock<std::mutex> locker(l_mutex);
        locker.unlock();

        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

        if (l_toDay != t.tm_mday) {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", l_path, tail, l_suffix);
            l_toDay = t.tm_mday;
            l_lineCnt = 0;
        } else {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", l_path, tail, (l_lineCnt / MAX_LINES), l_suffix);
        }

        locker.lock();
        Flush();
        fclose(l_fp);
        l_fp = fopen(newFile, "a");
        assert(l_fp != nullptr);
    }

    {
        std::unique_lock<std::mutex> locker(l_mutex);
        ++l_lineCnt;
        int n = snprintf(l_buff.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                         t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                         t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);

        l_buff.HasWritten(n);
        AppendLogLevelTitle(level);

        va_start(vaList, format);
        int m = vsnprintf(l_buff.BeginWrite(), l_buff.WritableBytes(), format, vaList);
        va_end(vaList);

        l_buff.HasWritten(m);
        l_buff.Append("\n\0", 2);

        if (isAsync && l_blQue && !l_blQue->Full()) {
            l_blQue->Push_back(l_buff.RetrieveAllToStr());
        } else {
            fputs(l_buff.Peek(), l_fp);
        }
        l_buff.RetrieveAll();
    }
}

void Log::AppendLogLevelTitle(int level) {
    switch (level) {
        case 0:
            l_buff.Append("[debug]: ", 9);
            break;
        case 1:
            l_buff.Append("[info] : ", 9);
            break;
        case 2:
            l_buff.Append("[warn] : ", 9);
            break;
        case 3:
            l_buff.Append("[error]: ", 9);
            break;
        default:
            l_buff.Append("[info] : ", 9);
            break;
    }
}

void Log::Flush() {
    if (isAsync) {
        l_blQue->Flush();
    }
    fflush(l_fp);
}

void Log::AsyncWrite() {
    std::string str;
    while (l_blQue->Pop(str)) {
        std::lock_guard<std::mutex> locker(l_mutex);
        fputs(str.c_str(), l_fp);
    }
}

Log *Log::Instance() {
    static Log inst;
    return &inst;
}

void Log::FlushLogThread() {
    Log::Instance()->AsyncWrite();
}