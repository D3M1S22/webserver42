#pragma once

#include "Error.hpp"
#include "Server.hpp"
#include "Utils.hpp"
#include <fstream>
#include <iostream>
#include <istream>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
class ARules;
class Server;
enum LangCgi { PYTHON, GO };

enum StatusCodes {
  NOT_ALLOWED = 405,
  BAD_REQUEST = 400,
  FORBIDDEN = 403,
  NOT_FOUND = 400,
  SERVER_ERROR = 500,
  REDIRECTION = 302,
  OK = 200
};

class RequestHandler {
private:
  std::string _method;
  std::string _path;
  std::string _body;
  std::string _hostname;
  int _contentSize;
  std::string _boundary;
  std::string _responseHeader;
  std::string _responseBody;
  int _responseStatus;
  std::string _fileName;
  reqStatusParams _reqStatus;
  RequestHandler();

public:
  const std::string getMethod() const;
  std::string &getPath();
  const std::string getBody() const;
  RequestHandler(const int &fd);
  void check(ARules &s);
  void sendResp(const int &fd);
  void setAllowed(const bool &a);
  reqStatusParams getReqStatus() const;

  void error(int fd, std::string page);

  void get(const std::string indexFile);

  void post(std::string completePath);

  bool deleteR();

  void setBody(const std::string &body);

  int getResponseStatus() const;

  void createAutoindex(const std::string directory);

  void createHeaderResp(const std::string contentType = "text/html",
                        const std::string optionalHeaders = "");

  void handleCgi(Server *serv, int fd, int lang);

  void createResponse(Server *Serv, int fd);
  ~RequestHandler();
};
