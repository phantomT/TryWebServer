#include "sql_conn_pool.h"

SqlConnPool *SqlConnPool::Instance() {
    static SqlConnPool connPool;
    return &connPool;
}

void SqlConnPool::Init(const char *host, int port,
                       const char *user, const char *pwd, const char *dbName,
                       int connSize = 10) {
    assert(connSize > 0);
    for (int i = 0; i < connSize; ++i) {
        MYSQL *sql = nullptr;
        sql = mysql_init(sql);
        if (!sql) {
            LOG_ERROR("MySql Init error!")
            assert(sql);
        }
        sql = mysql_real_connect(sql, host,
                                 user, pwd,
                                 dbName, port, nullptr, 0);
        if (!sql) {
            LOG_ERROR("MySql Connect error!")
        }
        s_connQue.emplace(sql);
    }
    MAX_CONN = connSize;
    sem_init(&semId, 0, MAX_CONN);
}

MYSQL *SqlConnPool::GetConn() {
    MYSQL *sql = nullptr;
    if (s_connQue.empty()) {
        LOG_WARN("SqlConnPool busy!")
        return nullptr;
    }
    sem_wait(&semId);
    {
        std::lock_guard<std::mutex> locker(s_mutex);
        sql = s_connQue.front();
        s_connQue.pop();
    }
    return sql;
}

void SqlConnPool::FreeConn(MYSQL *sql) {
    assert(sql);
    std::lock_guard<std::mutex> locker(s_mutex);
    s_connQue.emplace(sql);
    sem_post(&semId);
}

void SqlConnPool::ClosePool() {
    std::lock_guard<std::mutex> locker(s_mutex);
    while (!s_connQue.empty()) {
        auto item = s_connQue.front();
        s_connQue.pop();
        mysql_close(item);
    }
    mysql_library_end();
}

int SqlConnPool::GetFreeConnCount() {
    std::lock_guard<std::mutex> locker(s_mutex);
    return s_connQue.size();
}

SqlConnPool::~SqlConnPool() {
    ClosePool();
}
