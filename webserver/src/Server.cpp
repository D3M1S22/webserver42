#include "../includes/Server.hpp"
#include "../includes/RequestHandler.hpp"
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <istream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
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
  _indexFile = s._indexFile;
  _defaultErrorPage = s._defaultErrorPage;
  _serverFd = s._serverFd;
  for (std::map<std::string, ARules *>::const_iterator it = s._location.begin();
       it != s._location.end(); ++it) {
    _location[it->first] =
        new Location(*(dynamic_cast<Location *>(it->second)));
  }
  return *this;
}

Server::Server(const std::string &serverConf) : ARules(0) {
  _serverFd = 1024;
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
}

int  Server::getFd() const {return _serverFd;}

void Server::loadDefaultFiles() {
  std::string path = "www" + _root + "/" + _index;
  std::ifstream s(path.c_str());
  if (!s.is_open()) {
    s.close();
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

void  Server::createSocket()
{
  _serverFd = socket(AF_INET, SOCK_STREAM, 0);  /*create socket fd*/
  std::cout << "fd server = " << _serverFd << std::endl;
  if ( _serverFd == -1)
    throw Error("error creating socket connection on server "+_serverName);

  // ADDED AFTER MAY CAUSE DAMAGE LOL
  int opt = 1;
  if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
      perror("setsockopt");
      exit(EXIT_FAILURE);
  }
  // //

  setNonBlocking(_serverFd); /*set to non block*/

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(_port);

  if (bind(_serverFd, (sockaddr*)&address, sizeof(address)) == -1)
    throw Error("error binding socket to port on server "+_serverName);

  if (listen(_serverFd, 10) == -1)
  {
    close (_serverFd);
    throw Error("error listening on server "+_serverName);
  }
  std::cout << "server listening on port " << _port << std::endl;
}



void Server::handleClient(int clientFd) {
  RequestHandler rq(clientFd);
  rq.check(*(dynamic_cast<ARules *>(this)), clientFd);
  if (rq.getPath().find(".py") != std::string::npos)
    rq.handleCgi(this, clientFd, 0);
  else if (rq.getPath().find(".go") != std::string::npos)
    rq.handleCgi(this, clientFd, 1);
  else
    rq.createResponse(this, clientFd);
  // std::map<std::string, std::string>::iterator errorP;
  // if (!((rq.getReqStatus() & 1) >> 0)) {
  //   errorP = _errorPage.find(Utils::to_string(NOT_ALLOWED));
  //   rq.error(NOT_ALLOWED,
  //            (errorP != _errorPage.end() ? errorP->second : _defaultErrorPage),
  //            clientFd);
  // }
  

  const char *responseHeader = "HTTP/1.1 200 OK\r\n"
                               "Content-Type: text/html\r\n"
                               "Connection: keep-alive"
                               "\r\n";
  const char *responseBody = _indexFile.c_str();

  send(clientFd, responseHeader, ((std::string)responseHeader).length(), 0);

  // Send HTML body
  send(clientFd, responseBody, ((std::string)responseBody).length(), 0);

  // Close client connection
  close(clientFd);
  // close(clientFd);
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


std::string Server::getErrPage(int errNb){
  std::map<std::string, std::string>::iterator errorP = _errorPage.find(Utils::to_string(errNb));
  if(errorP != _errorPage.end())
  {
    std::ifstream s(("www"+_root+"/"+errorP->second).c_str());
    std::stringstream buffer;
    buffer << s.rdbuf();
    s.close();
    return (buffer.str()+"\r\n\r\n");
  }
  return _defaultErrorPage;
}

std::string Server::composedPath()
{
  return "www"+_root;
}

Server::~Server() {
  // close(_serverFd);
  for (std::map<std::string, ARules *>::iterator it = _location.begin();
       it != _location.end(); ++it) {
    if (it->second)
      delete it->second;
  }
  _location.clear();
}

void setNonBlocking(int servFd)
{
    int flags = fcntl(servFd, F_GETFL, 0);
    if (flags == -1)
        exit(EXIT_FAILURE);
    flags|= O_NONBLOCK;
    if (fcntl(servFd, F_SETFL, flags) == -1)
        exit(EXIT_FAILURE);
}

