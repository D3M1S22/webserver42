#include "../includes/RequestHandler.hpp"
#include "../includes/Error.hpp"
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

int RequestHandler::getResponseStatus() const { return _responseStatus; }

RequestHandler::~RequestHandler() {}
