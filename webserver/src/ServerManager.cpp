#include "../includes/ServerManager.hpp"
#include <cstddef>
#include <cstdlib>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <vector>

ServerManager::ServerManager()
{
    _epollFd = epoll_create1(0); 
}

int ServerManager::getEpollFd() const {return _epollFd;}

void  ServerManager::addServerToEpoll(std::vector<Server>& servers)
{
  std::vector<Server>::iterator it = servers.begin();
  std::vector<Server>::iterator ite = servers.end();
  for (; it != ite; it++) {
    it->createSocket();
    addFdToEpoll(it->getFd(), EPOLLIN);
    _servers.push_back(*it);
  }
}

void  ServerManager::addFdToEpoll(int fd, int flags)
{
  event.events = flags;
  event.data.fd = fd;
  if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &event) == -1)
    throw Error("error adding server fd to epoll");
}

void  ServerManager::handleNewClient(int fd)
{
  sockaddr_in addr;
  socklen_t addrLen = sizeof(addr);
  currentClientFd = accept(fd, (sockaddr*)&addr, &addrLen);
  if (currentClientFd == -1) {
    std::cerr << "Failed to accept new client on server with fd " << fd <<  std::endl;
    return ;
  }
  setNonBlocking(currentClientFd);
  addFdToEpoll(currentClientFd, EPOLLIN|EPOLLET);
  _clientsFds[currentClientFd] = fd;
}

Server*  ServerManager::isServerFd(int fd)
{
  std::vector<Server>::iterator it = _servers.begin();
  for(; it != _servers.end(); it++)
  {
    if (fd == it->getFd())
      return it.base();
  }
  return 0;
}

void  ServerManager::mainLoop()
{
  while(1)
  {
    int numEvents = epoll_wait(_epollFd, events, 10, -1);
    for (int i = 0; i < numEvents; i++)
    {
      Server *s = isServerFd(events[i].data.fd);
      if (s != 0)
      {
        handleNewClient(s->getFd());
        s->handleClient(currentClientFd);
      }
      else {
        int clientFd = events[i].data.fd;
        s = isServerFd(_clientsFds[clientFd]);
        s->handleClient(clientFd);
      }
    }
  }
}

ServerManager::~ServerManager(){
  std::cout << "fd num before closing " << _epollFd << std::endl;
  close(_epollFd);
}

