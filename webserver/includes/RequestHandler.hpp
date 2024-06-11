#pragma once

#include "ARules.hpp"
#include "Server.hpp"
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

class RequestHandler {
private:
  std::string _method;
  std::string _path;
  std::string _body;
  std::string _responseHeader;
  std::string _responseBody;
  bool _isAllowed;
  RequestHandler();

public:
  const std::string getMethod() const;
  const std::string getPath() const;
  const std::string getBody() const;
  RequestHandler(const int &fd);
  void checkMethod(ARules &s);
  void sendResp(const int &fd);
  void setAllowed(const bool &a);
  ~RequestHandler();
};
