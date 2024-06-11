#pragma once

#include "../includes/Error.hpp"
#include "../includes/Location.hpp"
#include "../includes/RequestHandler.hpp"
#include "ARules.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>

class Server : public ARules {
private:
  Server();
  int _serverFd;
  int _epollFd;
  std::string _serverName;
  std::map<std::string, std::string> _errorPage;
  std::string _indexFile;
  std::string _defaultErrorPage;
  int _port;
  void handleNewConnection(const epoll_event &event);
  void handleClientData(const epoll_event &event);
  void addEventToEpoll(int fd, uint32_t events);
  int create_tcp_server_socket();
  void loadDefaultFiles();

public:
  void handleClient(int clientFd);
  Server(const std::string &s);
  Server(const Server &s);
  Server &operator=(const Server &s);
  void printConf(const std::string &level) const;
  void start();
  // using ARules::getLocation;
  // using ARules::getMethods;
  ~Server();
};
