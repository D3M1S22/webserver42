#pragma once

#include "./Server.hpp"
#include <iostream>
#include <map>
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>

class ServerManager {
private:
  epoll_event event;
  epoll_event events[10000];
  int _epollFd;
  int currentClientFd;
  std::map<int, int> _clientsFds;
  std::vector<Server> _servers;

public:
  ServerManager();
  ~ServerManager();
  void mainLoop();
  Server *isServerFd(int fd);
  void handleNewClient(int fd);
  int getEpollFd() const;
  void addServerToEpoll(std::vector<Server> &servers);
  void addFdToEpoll(int fd, int flags);
};
