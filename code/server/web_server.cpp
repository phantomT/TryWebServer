#include "web_server.h"

WebServer::WebServer(
        int port, int trigMode, int timeoutMS, bool optLinger,
        int sqlPort, const char *sqlUser, const char *sqlPwd,
        const char *dbName, int connPoolNum, int threadNum,
        bool openLog, int logLevel, int logQueSize) :
        w_port(port), w_openLinger(optLinger), w_timeoutMs(timeoutMS), w_shutdown(false),
        w_timer(new Timer()), w_threadPool(new ThreadPool(threadNum)), w_epoller(new Epoller()) {
    w_srcDir = getcwd(nullptr, 256);
    assert(w_srcDir);
    strncat(w_srcDir, "/resources/", 16);
    HttpConn::userCount = 0;
    HttpConn::srcDir = w_srcDir;
    SqlConnPool::Instance()->Init("localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);

    InitEventMode(trigMode);
    if (!InitSocket()) { w_shutdown = true; }

    if (openLog) {
        Log::Instance()->Init(logLevel, "./log", ".log", logQueSize);
        if (w_shutdown) { LOG_ERROR("========== Server Init error!==========") }
        else {
            LOG_INFO("========== Server Init ==========")
            LOG_INFO("Port:%d, OpenLinger: %s", w_port, optLinger ? "true" : "false")
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                     (w_listenEvent & EPOLLET ? "ET" : "LT"),
                     (w_connEvent & EPOLLET ? "ET" : "LT"))
            LOG_INFO("LogSys level: %d", logLevel)
            LOG_INFO("srcDir: %s", HttpConn::srcDir)
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", connPoolNum, threadNum)
        }
    }
}

WebServer::~WebServer() {
    close(w_listenFd);
    w_shutdown = true;
    free(w_srcDir);
    SqlConnPool::Instance()->ClosePool();
}

void WebServer::InitEventMode(int trigMode) {
    w_listenEvent = EPOLLRDHUP;
    w_connEvent = EPOLLONESHOT | EPOLLRDHUP;
    switch (trigMode) {
        case 0:
            break;
        case 1:
            w_connEvent |= EPOLLET;
            break;
        case 2:
            w_listenEvent |= EPOLLET;
            break;
        default:
            w_listenEvent |= EPOLLET;
            w_connEvent |= EPOLLET;
            break;
    }
    HttpConn::isET = (w_connEvent & EPOLLET);
}

void WebServer::Start() {
    // epoll wait timeout == -1 无事件将阻塞
    int timeMS = -1;
    if (!w_shutdown) { LOG_INFO("========== Server start ==========") }
    while (!w_shutdown) {
        if (w_timeoutMs > 0) {
            timeMS = w_timer->GetNextTick();
        }
        int eventCnt = w_epoller->Wait(timeMS);
        for (int i = 0; i < eventCnt; i++) {
            // 处理事件
            int fd = w_epoller->GetEventFd(i);
            uint32_t events = w_epoller->GetEvents(i);
            if (fd == w_listenFd) {
                DealListen();
            } else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(w_users.count(fd) > 0);
                CloseConn(&w_users[fd]);
            } else if (events & EPOLLIN) {
                assert(w_users.count(fd) > 0);
                DealRead(&w_users[fd]);
            } else if (events & EPOLLOUT) {
                assert(w_users.count(fd) > 0);
                DealWrite(&w_users[fd]);
            } else {
                LOG_ERROR("Unexpected event")
            }
        }
    }
}

void WebServer::SendError(int fd, const char *info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if (ret < 0) {
        LOG_WARN("send error to client[%d] error!", fd)
    }
    close(fd);
}

void WebServer::CloseConn(HttpConn *client) {
    assert(client);
    LOG_INFO("Client[%d] quit!", client->GetFd())
    w_epoller->DelFd(client->GetFd());
    client->Close();
}

void WebServer::AddClient(int fd, sockaddr_in addr) {
    assert(fd > 0);
    w_users[fd].Init(fd, addr);
    if (w_timeoutMs > 0) {
        w_timer->Add(fd, w_timeoutMs, [this, w_userFd = &w_users[fd]] { CloseConn(w_userFd); });
    }
    w_epoller->AddFd(fd, EPOLLIN | w_connEvent);
    SetFdNonblock(fd);
    LOG_INFO("Client[%d] in!", w_users[fd].GetFd())
}

void WebServer::DealListen() {
    struct sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(w_listenFd, (struct sockaddr *) &addr, &len);
        if (fd <= 0) { return; }
        else if (HttpConn::userCount >= MAX_FD) {
            SendError(fd, "Server busy!");
            LOG_WARN("Clients is Full!")
            return;
        }
        AddClient(fd, addr);
    } while (w_listenEvent & EPOLLET);
}

void WebServer::DealRead(HttpConn *client) {
    assert(client);
    ExtentTime(client);
    w_threadPool->AddTask([this, client] { OnRead(client); });
}

void WebServer::DealWrite(HttpConn *client) {
    assert(client);
    ExtentTime(client);
    w_threadPool->AddTask([this, client] { OnWrite(client); });
}

void WebServer::ExtentTime(HttpConn *client) {
    assert(client);
    if (w_timeoutMs > 0) { w_timer->Adjust(client->GetFd(), w_timeoutMs); }
}

void WebServer::OnRead(HttpConn *client) {
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->Read(&readErrno);
    if (ret <= 0 && readErrno != EAGAIN) {
        CloseConn(client);
        return;
    }
    OnProcess(client);
}

void WebServer::OnProcess(HttpConn *client) {
    if (client->process()) {
        w_epoller->ModFd(client->GetFd(), w_connEvent | EPOLLOUT);
    } else {
        w_epoller->ModFd(client->GetFd(), w_connEvent | EPOLLIN);
    }
}

void WebServer::OnWrite(HttpConn *client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->Write(&writeErrno);
    if (client->ToWriteBytes() == 0) {
        // 传输完成
        if (client->IsKeepAlive()) {
            OnProcess(client);
            return;
        }
    } else if (ret < 0) {
        if (writeErrno == EAGAIN) {
            // 继续传输
            w_epoller->ModFd(client->GetFd(), w_connEvent | EPOLLOUT);
            return;
        }
    }
    CloseConn(client);
}

// Create listenFd
bool WebServer::InitSocket() {
    int ret;
    struct sockaddr_in addr{};
    if (w_port > 65535 || w_port < 1024) {
        LOG_ERROR("Port:%d error!", w_port)
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(w_port);
    struct linger optLinger = {0};
    if (w_openLinger) {
        // 优雅关闭: 直到所剩数据发送完毕或超时
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }

    w_listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (w_listenFd < 0) {
        LOG_ERROR("Create socket error!", w_port)
        return false;
    }

    ret = setsockopt(w_listenFd, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if (ret < 0) {
        close(w_listenFd);
        LOG_ERROR("Init linger error!", w_port)
        return false;
    }

    int optVal = 1;
    // 端口复用
    // 只有最后一个套接字会正常接收数据
    ret = setsockopt(w_listenFd, SOL_SOCKET, SO_REUSEADDR, (const void *) &optVal, sizeof(int));
    if (ret == -1) {
        LOG_ERROR("set socket setsockopt error !")
        close(w_listenFd);
        return false;
    }

    ret = bind(w_listenFd, (struct sockaddr *) &addr, sizeof(addr));
    if (ret < 0) {
        LOG_ERROR("Bind Port:%d error!", w_port)
        close(w_listenFd);
        return false;
    }

    ret = listen(w_listenFd, 6);
    if (ret < 0) {
        LOG_ERROR("Listen port:%d error!", w_port)
        close(w_listenFd);
        return false;
    }
    ret = w_epoller->AddFd(w_listenFd, w_listenEvent | EPOLLIN);
    if (ret == 0) {
        LOG_ERROR("Add listen error!")
        close(w_listenFd);
        return false;
    }
    SetFdNonblock(w_listenFd);
    LOG_INFO("Server port:%d", w_port)
    return true;
}

int WebServer::SetFdNonblock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}


