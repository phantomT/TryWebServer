#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cerrno>

#include "../log/log.h"
#include "../pool/sql_conn_RAII.h"
#include "../buffer/buffer.h"
#include "http_request.h"
#include "http_response.h"

class HttpConn {
public:
    HttpConn();

    ~HttpConn();

    void Init(int sockFd, const sockaddr_in &addr);

    ssize_t Read(int *saveErrno);

    ssize_t Write(int *saveErrno);

    void Close();

    int GetFd() const;

    int GetPort() const;

    const char *GetIP() const;

    sockaddr_in GetAddr() const;

    bool process();

    int ToWriteBytes() {
        return h_iov[0].iov_len + h_iov[1].iov_len;
    }

    bool IsKeepAlive() const {
        return h_request.IsKeepAlive();
    }

    static bool isET;
    static const char *srcDir;
    static std::atomic<int> userCount;

private:
    int h_fd;
    struct sockaddr_in s_addr;
    bool isClose;
    int h_iovCnt;
    struct iovec h_iov[2];
    Buffer h_readBuff; // 读缓冲区
    Buffer h_writeBuff; // 写缓冲区
    HttpRequest h_request;
    HttpResponse h_response;
};


#endif //HTTP_CONN_H