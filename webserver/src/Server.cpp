#include "../includes/Server.hpp"
#include <fstream>
#include <istream>
#include <string>
#include <unistd.h>

Server::Server() : ARules(0) {}

Server::Server(const Server &s) : ARules() { *this = s; }

Server &Server::operator=(const Server &s) {
  if (this == &s)
    return *this;
  _port = s._port;
  _errorPage = s._errorPage;
  _serverName = s._serverName;
  _autoIndex = s._autoIndex;
  _index = s._index;
  _root = s._root;
  _return = s._return;
  _methods = s._methods;
  _isLocation = s._isLocation;
  _clientSize = s._clientSize;
  _epollFd = s._epollFd;
  _serverFd = s._serverFd;
  _indexFile = s._indexFile;
  _defaultErrorPage = s._defaultErrorPage;
  for (std::map<std::string, ARules *>::const_iterator it = s._location.begin();
       it != s._location.end(); ++it) {
    _location[it->first] =
        new Location(*(dynamic_cast<Location *>(it->second)));
  }
  return *this;
}

int Server::create_tcp_server_socket() {
  const int opt = 1;
  int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd == -1) {
    Error("Could not create socket");
  }

  std::cout << "Created a socket with fd: " << fd << std::endl;

  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
    close(fd);
    throw Error("Could not set socket options");
  };
  struct sockaddr_in saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(_port);
  saddr.sin_addr.s_addr = INADDR_ANY;

  if (bind(fd, reinterpret_cast<struct sockaddr *>(&saddr),
           sizeof(struct sockaddr_in)) == -1) {
    close(fd);
    Error("Could not bind to socket");
  }

  if (listen(fd, 1000) == -1) {
    close(fd);
    Error("Could not listen on socket");
  }

  return fd;
}

void Server::addEventToEpoll(int fd, uint32_t events) {
  epoll_event event;
  event.events = events;
  event.data.fd = fd;
  if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &event) == -1) {
    std::cerr << "Failed to add socket to epoll instance." << std::endl;
    close(fd);
  }
}

void Server::loadDefaultFiles() {
  std::string path = "www" + _root + "/" + _index;
  std::ifstream s(path.c_str());
  if (!s.good()) {
    std::cout << "ERROR HANDLING INDEX FILE CHECK CONFIG FILE" << std::endl;
    std::cout << "LOADING THE DEFAULT index.html FROM /html/default/"
              << std::endl;
    s.open("www/default/index.html");
  }
  std::string line;
  while (std::getline(s, line)) {
    _indexFile += line + "\r\n";
  }
  _indexFile += "\r\n";
  s.close();
  s.open("www/default/error.html");
  while (std::getline(s, line)) {
    _defaultErrorPage += line + "\r\n";
  }
  _defaultErrorPage += "\r\n";
  s.close();
}

Server::Server(const std::string &serverConf) : ARules(0) {
  std::istringstream iss(serverConf);
  std::string line;
  std::string locationConf = "";
  while (!(std::getline(iss, line).eof())) {
    if (!line.empty()) {
      std::istringstream lineStream(line);
      std::string key, value;
      lineStream >> key;
      if (key == "listen") {
        std::string hostPort;
        lineStream >> hostPort;
        size_t colonPos = hostPort.find(':');
        std::stringstream s(hostPort.substr(colonPos + 1));
        s >> _port;
      } else if (key == "server_name") {
        lineStream >> _serverName;
      } else if (key == "error_page") {
        std::string errN;
        lineStream >> errN;
        lineStream >> _errorPage[errN];
      } else {
        parseConf(key, lineStream, iss);
      }
    }
  }
  loadDefaultFiles();
  _serverFd = create_tcp_server_socket();
  _epollFd = epoll_create1(0);
  if (_epollFd == -1) {
    close(_serverFd);
    Error("Failed to create epoll instance.");
  }
  addEventToEpoll(_serverFd, EPOLLIN);
  std::cout << "Server started. Listening on port " << this->_port << std::endl;
}

void Server::handleClient(int clientFd) {
  RequestHandler rq(clientFd);
  std::string m = rq.getMethod();
  std::string p = rq.getPath();
  const char *responseHeader = "HTTP/1.1 200 OK\r\n"
                               "Content-Type: text/html\r\n"
                               "Connection: close\r\n"
                               "\r\n";
  const char *responseBody = _indexFile.c_str();
  // if (p.length() == 1 && p[0] == '/')
  //   rq.setAllowed(true);
  // else
  rq.checkMethod(*(dynamic_cast<ARules *>(this)));
  send(clientFd, responseHeader, ((std::string)responseHeader).length(), 0);

  // Send HTML body
  send(clientFd, responseBody, ((std::string)responseBody).length(), 0);

  // Close client connection
  close(clientFd);
  // close(clientFd);
}

void Server::handleNewConnection(const epoll_event &event) {
  (void)event;
  struct sockaddr_in clientAddress;
  socklen_t clientAddressLength = sizeof(clientAddress);
  int clientFd = accept(_serverFd, (struct sockaddr *)&clientAddress,
                        &clientAddressLength);
  if (clientFd == -1) {
    std::cerr << "Failed to accept client connection." << std::endl;
    return;
  }
  addEventToEpoll(clientFd, EPOLLIN);
  handleClient(clientFd);
}

void Server::handleClientData(const epoll_event &event) {
  int clientFd = event.data.fd;
  handleClient(clientFd);
}

void Server::start() {
  while (true) {
    epoll_event events[10];
    int numEvents = epoll_wait(_epollFd, events, 10, -1);
    // if (numEvents == -1) {
    //   std::cerr << "Failed to wait for events." << std::endl;
    //   break;
    // }
    for (int i = 0; i < numEvents; ++i) {
      if (events[i].data.fd == _serverFd) {
        handleNewConnection(events[i]);
      } else {
        handleClientData(events[i]);
      }
    }
  }
  close(_serverFd);
  close(_epollFd);
}

void Server::printConf(const std::string &level) const {
  (void)level;
  std::cout << " SERVER CONF CHECK " << std::endl;
  std::cout << "port " << _port << std::endl;
  std::cout << "servername " << _serverName << std::endl;
  std::cout << "root " << _root << std::endl;
  std::cout << "Return " << _return << std::endl;
  for (std::map<std::string, std::string>::const_iterator it =
           _errorPage.begin();
       it != _errorPage.end(); it++)
    std::cout << "Error Page " << it->first << " " << it->second << std::endl;
  for (std::vector<std::string>::const_iterator it = _methods.begin();
       it != _methods.end(); it++)
    std::cout << "Method " << *it << std::endl;
  std::cout << "clientSize " << _clientSize << std::endl;
  std::cout << "autoIndex " << _autoIndex << std::endl;
  for (std::map<std::string, ARules *>::const_iterator it = _location.begin();
       it != _location.end(); ++it) {
    std::cout << "Location " << it->first << std::endl;
    it->second->printConf("\t");
  }
  std::cout << std::endl;
  std::cout << "index " << _index << std::endl;
  std::cout << "isLocation " << _isLocation << std::endl;
  std::cout << " END OF SERVER CONF " << std::endl;
}

Server::~Server() {
  for (std::map<std::string, ARules *>::iterator it = _location.begin();
       it != _location.end(); ++it) {
    if (it->second)
      delete it->second;
  }
  _location.clear();
}
