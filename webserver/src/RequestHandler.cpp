#include "../includes/RequestHandler.hpp"
#include "../includes/Error.hpp"
#include <cerrno>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <poll.h>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <fcntl.h>
#include <iomanip>
#include <ios>
#include <istream>
#include <map>
#include <sstream>
#include <streambuf>
#include <string>
#include <valarray>
#include <vector>

// Function to generate a random string of specified length
std::string generateRandomString(int length) {
  const char charset[] =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  const int max_index = sizeof(charset) - 1;

  std::string random_string;
  srand(static_cast<unsigned int>(time(0))); // Seed the random number generator

  for (int i = 0; i < length; ++i) {
    random_string += charset[rand() % max_index];
  }

  return random_string;
}

#define BUFFER_SIZE 4096
RequestHandler::RequestHandler() {}

// Function to find the position of a substring
size_t findBoundary(const std::string &data, const std::string &boundary,
                    size_t start) {
  return data.find(boundary, start);
}

RequestHandler::RequestHandler(const int &fd)
    : _body(""), _responseStatus(200), _contentSize(0) {
  char buffer[2];
  ssize_t bytesRead;
  while ((bytesRead = recv(fd, buffer, 1, 0)) == -1)
    continue;
  buffer[bytesRead] = '\0';
  std::vector<char> resp;
  resp.insert(resp.end(), buffer, buffer + bytesRead);
  while ((bytesRead = recv(fd, buffer, 1, 0)) > 0) {
    resp.insert(resp.end(), buffer, buffer + bytesRead);
  }
  std::string response(resp.begin(), resp.end());
  int pos = response.find("\r\n\r\n");
  std::string header = response.substr(0, pos);
  std::istringstream firstLine(header.substr(0, header.find("\r\n")));
  firstLine >> _method >> _path;
  size_t contentLengthPos = header.find("Content-Length:");
  std::string contentLengthLine;
  if (contentLengthPos != std::string::npos) {
    std::string contentLengthLineStr = header.substr(contentLengthPos);
    contentLengthLine =
        contentLengthLineStr.substr(0, contentLengthLineStr.find("\r\n"));
    _body = response.substr(pos + 4);
  }
  std::string e = "boundary=";
  size_t firstWPos = header.find(e);
  if (firstWPos != std::string::npos) {
    _boundary = header.substr(firstWPos + e.length(),
                              header.find("\r", firstWPos + e.length()) -
                                  (firstWPos + e.length()));
  }
  std::istringstream s(contentLengthLine);
  if (s.fail()) {
    _responseStatus = SERVER_ERROR;
    return;
  }
  std::string key;
  s >> key;
  if (key == "Content-Length:") {
    s >> _contentSize;
  }
  if (_boundary.length() > 0 && _contentSize != 0) {
    std::string fileNameLocStr = "filename=\"";
    size_t fileNamePos = response.find(fileNameLocStr);
    if (fileNamePos != std::string::npos)
      _fileName = response.substr(
          fileNamePos + fileNameLocStr.length(),
          (response.find("\"", fileNamePos + fileNameLocStr.length())) -
              (fileNamePos + fileNameLocStr.length()));
  }
}

const std::string RequestHandler::getBody() const { return _body; }
const std::string RequestHandler::getMethod() const { return _method; }
std::string &RequestHandler::getPath() { return _path; }

void RequestHandler::check(ARules &s) {
  _reqStatus = s.isAllowed(_path, _method, _contentSize);
  if (!((_reqStatus & 1) >> 0)) {
    _responseStatus = NOT_ALLOWED;
    return;
  } else if ((_reqStatus & 8) >> 7) {
    _responseStatus = BAD_REQUEST;
    return;
  }
}

int fd_is_valid(int fd) { return fcntl(fd, F_GETFD) != -1 || errno != EBADF; }

void RequestHandler::sendResp(const int &fd) {
  send(fd, _responseHeader.c_str(), (_responseHeader).length(), 0);
  send(fd, _responseBody.c_str(), (_responseBody).length(), 0);
  close(fd);
}

int RequestHandler::getReqStatus() const { return _reqStatus; }

void RequestHandler::error(int fd, std::string page) {
  createHeaderResp("Connection: close\r\n");
  std::string fixedPage = Utils::replacestring(
      page, "{{errNbr}}", Utils::to_string(_responseStatus));
  _responseBody = fixedPage;
  sendResp(fd);
}

void RequestHandler::get(const std::string fullPath) {
  std::ifstream s(fullPath.c_str());
  if (!s.is_open()) {
    _responseStatus = NOT_FOUND;
    return;
  }
  _responseBody.clear();
  std::stringstream buffer;
  buffer << s.rdbuf();
  _responseBody = buffer.str() + "\r\n\r\n";
  s.close();
  _responseStatus = OK;
  createHeaderResp("");
}

void RequestHandler::post(std::string completePath) {
  std::string subBody = _body;
  size_t start = subBody.find(_boundary);
  size_t end = subBody.find(_boundary, (_boundary.length() + 2));
  if (start != std::string::npos && end != std::string::npos) {
    // Adjust positions to get the content in between
    start += _boundary.length() + 2; // Move past boundary and \r\n
    std::string headers =
        _body.substr(start, _body.find("\r\n\r\n", start) - start);
    start += headers.length() + 4; // Move past headers and \r\n\r\n
    //   // Extract binary data
    std::string fileContent =
        _body.substr(start, end - start - 2); // Exclude trailing \r\n
    //   // Write binary data to file
    _fileName.insert(0, generateRandomString(15));
    _fileName.insert(0, completePath);
    std::ofstream outFile(_fileName.c_str(),
                          std::ios::out | std::ios::binary | std::ios::trunc);
    if (outFile) {
      outFile.write(fileContent.c_str(), fileContent.length());
      outFile.close();
      std::cout << "File extracted successfully.\n";
    } else {
      std::cerr << "Error writing to file.\n";
    }
  }
  createHeaderResp("");
}

bool RequestHandler::deleteR() { return true; }

void RequestHandler::createResponse(Server *serv, int fd) {
  if (_method == "GET" && _responseStatus == 200) {
    if (_path.find(".") == std::string::npos) {
      if (_path[_path.length() - 1] != '/')
        _path.append("/");
      get(serv->composedPath() + _path + serv->getLocationIndex(_path));
    } else
      get(serv->composedPath() + _path);
  }
  if (_method == "POST") {
    if (_path.find("/cgi-bin/"))
      std::cout << "handle cgi" << std::endl;
    else
      post(serv->composedPath() + _path + "/uploadedFiles/");
    _responseBody = "OK";
  }
  if (_responseStatus == OK) {
    std::cout << "Sending response" << std::endl;
    sendResp(fd);
  } else {
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

void RequestHandler::setBody(const std::string &body) { _body = body; }

void RequestHandler::createHeaderResp(const std::string optionalHeaders) {
  _responseHeader += "HTTP/1.1 " + Utils::to_string(_responseStatus) +
                     (_responseStatus == OK ? " OK" : " KO") +
                     "\r\n"
                     "Content-Type: text/html\r\n"
                     "Connection: keep-alive" +
                     optionalHeaders +
                     "\r\n"
                     "\r\n";
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


int RequestHandler::getResponseStatus() const { return _responseStatus; }

RequestHandler::~RequestHandler() {}
