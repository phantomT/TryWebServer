#include "epoller.h"

Epoller::Epoller(int maxEvent): e_epollFd(epoll_create(512)), e_events(maxEvent){
    assert(e_epollFd >= 0 && !e_events.empty());
}

Epoller::~Epoller() {
    close(e_epollFd);
}

bool Epoller::AddFd(int fd, uint32_t events) const {
    if(fd < 0) return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(e_epollFd, EPOLL_CTL_ADD, fd, &ev);
}

bool Epoller::ModFd(int fd, uint32_t events) const {
    if(fd < 0) return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(e_epollFd, EPOLL_CTL_MOD, fd, &ev);
}

bool Epoller::DelFd(int fd) const {
    if(fd < 0) return false;
    epoll_event ev = {0};
    return 0 == epoll_ctl(e_epollFd, EPOLL_CTL_DEL, fd, &ev);
}

int Epoller::Wait(int timeoutMs) {
    return epoll_wait(e_epollFd, &e_events[0], static_cast<int>(e_events.size()), timeoutMs);
}

int Epoller::GetEventFd(size_t i) const {
    assert(i < e_events.size() && i >= 0);
    return e_events[i].data.fd;
}

uint32_t Epoller::GetEvents(size_t i) const {
    assert(i < e_events.size() && i >= 0);
    return e_events[i].events;
}