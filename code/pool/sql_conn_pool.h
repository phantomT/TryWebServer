#ifndef SQL_CONN_POOL_H
#define SQL_CONN_POOL_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>

#include "../log/log.h"

class SqlConnPool {
public:
    static SqlConnPool *Instance();

    MYSQL *GetConn();

    void FreeConn(MYSQL *conn);

    int GetFreeConnCount();

    void Init(const char *host, int port,
              const char *user, const char *pwd,
              const char *dbName, int connSize);

    void ClosePool();

private:
    ~SqlConnPool();

    int MAX_CONN;

    std::queue<MYSQL *> s_connQue;
    std::mutex s_mutex;
    sem_t semId;
};

#endif // SQL_CONN_POOL_H