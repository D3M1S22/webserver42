#include "../includes/RequestHandler.hpp"
#include <cerrno>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <poll.h>
#include <cstddef>
#include <cstring>
#include <map>
#include <sstream>
#include <vector>
#include "../includes/Error.hpp"

#define BUFFER_SIZE 4096
RequestHandler::RequestHandler() {}

RequestHandler::RequestHandler(const int &fd) : _responseStatus(200), _contentSize(0) {
  char buffer[BUFFER_SIZE];

  // Read the incoming data from the client socket
  ssize_t bytesRead;
  while ((bytesRead = recv(fd, buffer, BUFFER_SIZE, 0)) == -1)
    continue;
  if (bytesRead == 0) {
    std::cerr << "Client disconnected." << std::endl;
    close(fd);
    return;
  }
  std::string request = buffer;
  // std::cout << request << std::endl;
  buffer[bytesRead] = '\0';
  // std::cout << request.find("boundary=") << std::endl;
  int pos = request.find("\r\n\r\n");
  _body = request.substr(pos+4);
  std::string header =  request.substr(0, pos);
  std::string lastLine = header.substr(header.find_last_of("\n")+1);
  std::istringstream firstLine( header.substr(0, header.find("\n")));
  firstLine >> _method >> _path;
  // std::cout << _method << " " << _path << std::endl;
  std::istringstream s(lastLine);
  std::string key;
  s>>key;
  if(key == "Content-Length:")
    s >> _contentSize;
}

const std::string RequestHandler::getBody() const { return _body; }
const std::string RequestHandler::getMethod() const { return _method; }
std::string &RequestHandler::getPath() { return _path; }

void RequestHandler::check(ARules &s,int fd) {
  _reqStatus = s.isAllowed(_path, _method, _contentSize);
  if(!((_reqStatus & 1) >> 0))
  {
    _responseStatus = NOT_ALLOWED;
    return;
  }
  else if(((_reqStatus & 2) >> 1) && ((int)_body.length()< _contentSize))
  {
    int sizeToRead = _contentSize-_body.length() +1;
    char buffer[sizeToRead];
    int byteRead =-1;
    if(!(byteRead= recv(fd, buffer, sizeToRead, 0)))
    {
      std::cerr << "ERROR READING FILE "<< sizeToRead<< " "<< fd << std::endl;
      close(fd);
    }
    buffer[byteRead] = '\0';
    _body.append(buffer);
      // std::cout << "Buffer " << buffer << std::endl;
    return;
  }else if(!((_reqStatus & 2) >> 1)){
    // std::cout << "QUIII" << std::endl;
    _responseStatus = BAD_REQUEST;
    return;
  }
}

void RequestHandler::sendResp(const int &fd) {
  send(fd, _responseHeader.c_str(), (_responseHeader).length(), 0);
  send(fd, _responseBody.c_str(), (_responseBody).length(), 0);
  close(fd);
}

int RequestHandler::getReqStatus() const { return _reqStatus; }

void RequestHandler::error(int fd, std::string page) {
  createHeaderResp("Connection: close\r\n");
  std::string fixedPage = Utils::replacestring(page, "{{errNbr}}", Utils::to_string(_responseStatus));
  _responseBody = fixedPage;
  sendResp(fd);
}

void RequestHandler::get(const std::string fullPath) {
  std::ifstream s(fullPath.c_str());
  // std::cout << fullPath << std::endl;
  if (!s.is_open()) {
    _responseStatus = NOT_FOUND;
    return;
  }
  _responseBody.clear();
  std::stringstream buffer;
  buffer << s.rdbuf();
  _responseBody = buffer.str()+"\r\n\r\n";
  s.close();
  _responseStatus = OK;
  createHeaderResp("");
}

bool RequestHandler::post() {return true;}

bool RequestHandler::deleteR() {return true;}

void RequestHandler::createResponse(Server *serv, int fd) {
  std::cout << serv->composedPath()+_path+serv->getLocationIndex(_path) << "method = " << _method << std::endl;
  if(_method == "GET" && _responseStatus == 200)
  {
    if(_path.find(".") == std::string::npos){
      if(_path[_path.length()-1] != '/')
        _path.append("/");
      get(serv->composedPath()+_path+serv->getLocationIndex(_path));
    }
    else{
      get(serv->composedPath()+_path);
    }
  }
  if(_method == "POST"){
    // std::cout << << std::endl;
    // std::cout << "body = " <<_body<< " content size "<< _contentSize << std::endl;
  }
  if(_responseStatus == OK)
  {
    std::cout << "Sending response" << std::endl;
    sendResp(fd);
  }else{
    error(fd, serv->getErrPage(_responseStatus));
  }
}

int waitpid_with_timeout(pid_t pid, int* status, int timeout) {
    int elapsed_time = 0;
    while (elapsed_time < timeout * 10000) {  // Convert seconds to milliseconds
        int result = waitpid(pid, status, WNOHANG);
        if (result != 0) {
            return 0;
        }
        poll(NULL, 0, 100);  // Sleep for 100 milliseconds
        elapsed_time += 100;
    }
    std::cout << "error: timeout. killing pid " << std::endl;
    return 1;
}

void  RequestHandler::handleCgi(Server* serv,const int clientFd, int lang)
{
  std::string Cmd;
  char *argv[4];
  std::string arg = (serv->composedPath()+_path);
  if (lang == 0){
    if (arg.find("/upload.py") != std::string::npos)
      argv[2] = (char*)Utils::to_string(clientFd).c_str();
    else
      argv[2] = NULL;
    argv[1] = (char *)arg.c_str();
    Cmd = "/usr/bin/python3";
  }
  else
  {
    argv[1] = (char *)"run";
    argv[2] = (char *)arg.c_str();
    Cmd = "/usr/bin/go";

  }
  argv[0] = (char*)Cmd.c_str();
  argv[3] = NULL;


  if (access(("/"+arg).c_str(),R_OK | X_OK))
  {
    int fd[2];
    if (pipe(fd) == -1)
      std::cout << "error opening pipe" << std::endl;
    int pid = fork();
    if (!pid)
    {
      close(fd[0]);
      std::cout << "starting exec" << std::endl;
      if(dup2(fd[1], 1) == -1 || close(fd[1]) == 1)
        std::cout << "error duplicating file" << std::endl;
      execve(Cmd.c_str(), argv, environ);
      std::cout << "exec error: " << errno << std::endl;
      exit(0);
    }
    int status;
    waitpid_with_timeout(pid, &status, 1);
    kill(pid, 9);
    close(fd[1]);
    char buffer[1024];
    std::string output;
    size_t byteRead;
    while((byteRead = read(fd[0], buffer, 1024-1)) > 0){
      buffer[byteRead] = '\0';
      output += buffer;
    }
    close(fd[0]);
    _responseBody = Utils::to_string(buffer);
    createHeaderResp("");
    sendResp(clientFd);
  }
  else
   std::cout << "file cgi not found" << std::endl;
}

void RequestHandler::setBody(const std::string &body) {
  _body = body;
}

void RequestHandler::createHeaderResp(const std::string optionalHeaders){
  _responseHeader += "HTTP/1.1 "+ Utils::to_string(_responseStatus) + (_responseStatus == OK ? " OK" : " KO")+"\r\n"
                    "Content-Type: text/html\r\n"
                    "Connection: keep-alive"
                    +optionalHeaders+"\r\n"
                    "\r\n";
}

int RequestHandler::getResponseStatus()const{return _responseStatus;}

RequestHandler::~RequestHandler() {}
