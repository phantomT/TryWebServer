#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <cerrno>
#include <unordered_map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "../log/log.h"
#include "../timer/timer.h"
#include "../pool/sql_conn_pool.h"
#include "../pool/thread_pool.h"
#include "../pool/sql_conn_RAII.h"
#include "../http/http_conn.h"

class WebServer {
public:
    WebServer(
            int port, int trigMode, int timeoutMS, bool optLinger,
            int sqlPort, const char *sqlUser, const char *sqlPwd,
            const char *dbName, int connPoolNum, int threadNum,
            bool openLog, int logLevel, int logQueSize);

    ~WebServer();

    void Start();

private:
    bool InitSocket();

    void InitEventMode(int trigMode);

    void AddClient(int fd, sockaddr_in addr);

    void DealListen();

    void DealWrite(HttpConn *client);

    void DealRead(HttpConn *client);

    void SendError(int fd, const char *info);

    void ExtentTime(HttpConn *client);

    void CloseConn(HttpConn *client);

    void OnRead(HttpConn *client);

    void OnWrite(HttpConn *client);

    void OnProcess(HttpConn *client);

    static const int MAX_FD = 65536;

    static int SetFdNonblock(int fd);

    int w_port;
    bool w_openLinger;
    int w_timeoutMs;
    bool w_shutdown;
    int w_listenFd;
    char *w_srcDir;

    uint32_t w_listenEvent;
    uint32_t w_connEvent;

    std::unique_ptr<Timer> w_timer;
    std::unique_ptr<ThreadPool> w_threadPool;
    std::unique_ptr<Epoller> w_epoller;
    std::unordered_map<int, HttpConn> w_users;
};


#endif //WEB_SERVER_H