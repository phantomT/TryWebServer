#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <cerrno>
#include <vector>

class Epoller {
public:
    explicit Epoller(int maxEvent = 1024);

    ~Epoller();

    bool AddFd(int fd, uint32_t events) const;

    bool ModFd(int fd, uint32_t events) const;

    bool DelFd(int fd) const;

    int Wait(int timeoutMs = -1);

    int GetEventFd(size_t i) const;

    uint32_t GetEvents(size_t i) const;

private:
    int e_epollFd;
    std::vector<struct epoll_event> e_events;
};

#endif //EPOLLER_H