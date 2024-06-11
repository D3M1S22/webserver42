#pragma once

#include <iostream>
#include "./Server.hpp"
#include <map>
#include <sys/epoll.h>
#include <vector>

class ServerManager {
    private:
        epoll_event event;
        epoll_event events[10];
        int _epollFd;
        int currentClientFd;
        std::map<int, int> _clientsFds;
        std::vector<Server> _servers;
    public:
        ServerManager();
        ~ServerManager();
        void  mainLoop();
        Server*  isServerFd(int fd);
        void  handleNewClient(int fd);
        int   getEpollFd() const;
        void  addServerToEpoll(std::vector<Server>& servers);
        void  addFdToEpoll(int fd, int flags);
};