#include "http_conn.h"

const char *HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;
bool HttpConn::isET;

HttpConn::HttpConn() {
    h_fd = -1;
    s_addr = {0};
    isClose = true;
}

HttpConn::~HttpConn() {
    Close();
}

void HttpConn::Init(int sockFd, const sockaddr_in &addr) {
    assert(sockFd > 0);
    ++userCount;
    s_addr = addr;
    h_fd = sockFd;
    h_writeBuff.RetrieveAll();
    h_readBuff.RetrieveAll();
    isClose = false;
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", h_fd, GetIP(), GetPort(), (int) userCount)
}

void HttpConn::Close() {
    h_response.UnmapFile();
    if (!isClose) {
        isClose = true;
        userCount--;
        close(h_fd);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", h_fd, GetIP(), GetPort(), (int) userCount)
    }
}

int HttpConn::GetFd() const {
    return h_fd;
}

struct sockaddr_in HttpConn::GetAddr() const {
    return s_addr;
}

const char *HttpConn::GetIP() const {
    return inet_ntoa(s_addr.sin_addr);
}

int HttpConn::GetPort() const {
    return s_addr.sin_port;
}

ssize_t HttpConn::Read(int *saveErrno) {
    ssize_t len = -1;
    do {
        len = h_readBuff.ReadFd(h_fd, saveErrno);
        if (len <= 0) {
            break;
        }
    } while (isET);
    return len;
}

ssize_t HttpConn::Write(int *saveErrno) {
    ssize_t len = -1;
    do {
        len = writev(h_fd, h_iov, h_iovCnt);
        if (len <= 0) {
            *saveErrno = errno;
            break;
        }
        if (h_iov[0].iov_len + h_iov[1].iov_len == 0) { break; } /* 传输结束 */
        else if (static_cast<size_t>(len) > h_iov[0].iov_len) {
            h_iov[1].iov_base = (uint8_t *) h_iov[1].iov_base + (len - h_iov[0].iov_len);
            h_iov[1].iov_len -= (len - h_iov[0].iov_len);
            if (h_iov[0].iov_len) {
                h_writeBuff.RetrieveAll();
                h_iov[0].iov_len = 0;
            }
        } else {
            h_iov[0].iov_base = (uint8_t *) h_iov[0].iov_base + len;
            h_iov[0].iov_len -= len;
            h_writeBuff.Retrieve(len);
        }
    } while (isET || ToWriteBytes() > 10240);
    return len;
}

bool HttpConn::process() {
    h_request.Init();
    if (h_readBuff.ReadableBytes() <= 0) {
        return false;
    } else if (h_request.Parse(h_readBuff)) {
        LOG_DEBUG("%s", h_request.Path().c_str())
        h_response.Init(srcDir, h_request.Path(), h_request.IsKeepAlive(), 200);
    } else {
        h_response.Init(srcDir, h_request.Path(), false, 400);
    }

    h_response.MakeResponse(h_writeBuff);
    // 响应头
    h_iov[0].iov_base = const_cast<char *>(h_writeBuff.Peek());
    h_iov[0].iov_len = h_writeBuff.ReadableBytes();
    h_iovCnt = 1;

    // 文件
    if (h_response.FileLen() > 0 && h_response.File()) {
        h_iov[1].iov_base = h_response.File();
        h_iov[1].iov_len = h_response.FileLen();
        h_iovCnt = 2;
    }
    LOG_DEBUG("filesize:%d, %d  to %d", h_response.FileLen(), h_iovCnt, ToWriteBytes())
    return true;
}
