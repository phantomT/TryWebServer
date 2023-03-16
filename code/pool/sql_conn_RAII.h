#ifndef SQL_CONN_RAII_H
#define SQL_CONN_RAII_H
#include "sql_conn_pool.h"

// 资源在对象构造初始化 资源在对象析构时释放
class SqlConnRAII {
public:
    SqlConnRAII(MYSQL** sql, SqlConnPool *connPool) {
        assert(connPool);
        *sql = connPool->GetConn();
        raii_sql = *sql;
        raii_connPool = connPool;
    }
    
    ~SqlConnRAII() {
        if(raii_sql) { raii_connPool->FreeConn(raii_sql); }
    }
    
private:
    MYSQL *raii_sql;
    SqlConnPool* raii_connPool;
};

#endif //SQL_CONN_RAII_H