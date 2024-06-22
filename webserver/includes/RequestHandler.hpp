#pragma once

#include "ARules.hpp"
#include "Utils.hpp"
#include "Server.hpp"
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include "Error.hpp"
#include <fstream>

enum LangCgi {
  PYTHON, GO
};

enum StatusCodes {
  NOT_ALLOWED = 405,
  BAD_REQUEST = 400,
  FORBIDDEN = 403,
  NOT_FOUND = 400,
  OK = 200
};

class RequestHandler {
private:
  std::string _method;
  std::string _path;
  std::string _body;
  std::string _responseHeader;
  std::string _responseBody;
  int _responseStatus;
  int _contentSize;
  int _reqStatus;
  RequestHandler();

public:
  const std::string getMethod() const;
  std::string &getPath();
  const std::string getBody() const;
  RequestHandler(const int &fd);
  void check(ARules &s, int fd);
  void sendResp(const int &fd);
  void setAllowed(const bool &a);
  int getReqStatus() const;

  void error(int fd, std::string page);

  void get(const std::string indexFile);

  bool post();

  bool deleteR();

  void setBody(const std::string &body);

  int getResponseStatus() const;

  void createHeaderResp(const std::string optionalHeader);

  void handleCgi(Server* serv, int fd, int lang);

void createResponse(Server *Serv, int fd);
  ~RequestHandler();
};
