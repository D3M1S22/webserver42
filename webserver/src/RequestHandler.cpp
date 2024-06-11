#include "../includes/RequestHandler.hpp"
#include <map>
#include <vector>

RequestHandler::RequestHandler() {}

RequestHandler::RequestHandler(const int &fd) {
  const int BUFFER_SIZE = 1024;
  char buffer[BUFFER_SIZE];

  // Read the incoming data from the client socket
  int bytesRead;
  while((bytesRead = recv(fd, buffer, BUFFER_SIZE, 0)) == -1)  
    continue;
  if (bytesRead == 0) {
    std::cerr << "Client disconnected." << std::endl;
    close(fd);
    return;
  }
  buffer[bytesRead] = '\0';
  std::cout << buffer << std::endl;
  std::istringstream iss(buffer);
  std::string line;
  std::string null;
  std::string token;
  int i = 0;
  while (std::getline(iss, line)) {
    std::istringstream lineTokens(line);
    while (lineTokens >> token) {
      if (i == 0) {
        _method = token;
        i++;
      } else if (i == 1) {
        _path = token;
        i++;
      }
    }
  }
}

const std::string RequestHandler::getBody() const { return _body; }
const std::string RequestHandler::getMethod() const { return _method; }
const std::string RequestHandler::getPath() const { return _path; }

void RequestHandler::checkMethod(ARules &s) {
  _isAllowed = s.isAllowed(_path, _method);
}

void RequestHandler::sendResp(const int &fd) {
  send(fd, _responseHeader.c_str(), (_responseHeader).length(), 0);

  send(fd, _responseBody.c_str(), (_responseBody).length(), 0);
}

void RequestHandler::setAllowed(const bool &a) { _isAllowed = a; }

RequestHandler::~RequestHandler() {}
