#include "../includes/RequestHandler.hpp"
#include "../includes/Error.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <exception>
#include <fcntl.h>
#include <iomanip>
#include <ios>
#include <istream>
#include <map>
#include <poll.h>
#include <sstream>
#include <streambuf>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
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
RequestHandler::RequestHandler() : _reqStatus() {}

// Function to find the position of a substring
size_t findBoundary(const std::string &data, const std::string &boundary,
                    size_t start) {
  return data.find(boundary, start);
}

RequestHandler::RequestHandler(const int &fd)
    : _body(""), _contentSize(0), _responseStatus(200), _reqStatus() {
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
  std::string tmpPath = _path;
  if (_path.find(".") != std::string::npos)
    tmpPath = _path.substr(0, _path.find_last_of("/") + 1);
  s.isAllowed(tmpPath, _method, _contentSize, _reqStatus);
  if (!((_reqStatus._allACs & 1) >> 0)) {
    _responseStatus = NOT_ALLOWED;
    return;
  } else if ((_reqStatus._allACs & 8) >> 3) {
    _responseStatus = BAD_REQUEST;
    return;
  } else if (_reqStatus._redir.length() > 0)
    _responseStatus = REDIRECTION;
}

int fd_is_valid(int fd) { return fcntl(fd, F_GETFD) != -1 || errno != EBADF; }

void RequestHandler::sendResp(const int &fd) {
  send(fd, _responseHeader.c_str(), (_responseHeader).length(), 0);
  send(fd, _responseBody.c_str(), (_responseBody).length(), 0);
  close(fd);
}

reqStatusParams RequestHandler::getReqStatus() const { return _reqStatus; }

void RequestHandler::error(int fd, std::string page) {
  createHeaderResp("Connection: close\r\n");
  std::string fixedPage = Utils::replacestring(
      page, "{{errNbr}}", Utils::to_string(_responseStatus));
  _responseBody = fixedPage;
  sendResp(fd);
}

bool is_directory(const std::string &path) {
  struct stat statbuf;
  if (stat(path.c_str(), &statbuf) != 0) {
    return false;
  }
  return S_ISDIR(statbuf.st_mode);
}

void get_file_stats(const std::string &path, std::string &size_str,
                    std::string &mod_time_str) {
  struct stat statbuf;
  if (stat(path.c_str(), &statbuf) == 0) {
    // File size
    size_str = Utils::to_string(statbuf.st_size);

    // Modification time
    char mod_time[20];
    strftime(mod_time, sizeof(mod_time), "%Y-%m-%d %H:%M:%S",
             localtime(&statbuf.st_mtime));
    mod_time_str = mod_time;
  } else {
    size_str = "-";
    mod_time_str = "-";
  }
}

void RequestHandler::createAutoindex(const std::string directory) {
  std::vector<std::string> files;

  DIR *dirp = opendir(directory.c_str());
  if (dirp == NULL) {
    std::cerr << "Error: Unable to open directory " << directory << std::endl;
    _responseStatus = SERVER_ERROR;
    return;
  }

  struct dirent *dp;
  while ((dp = readdir(dirp)) != NULL) {
    std::string filename(dp->d_name);
    if (filename != "." && filename != "..") {
      files.push_back(filename);
    }
  }
  closedir(dirp);
  std::string html =
      "<html><head><title>Index of " + directory + "</title></head><body>";
  html += "<h1>Index of " + directory + "</h1>";
  html += "<table><tr><th>Name</th><th>Size</th><th>Last Modified</th></tr>";

  for (size_t i = 0; i < files.size(); ++i) {
    std::string file_path = directory + "/" + files[i];
    std::string size_str, mod_time_str;
    get_file_stats(file_path, size_str, mod_time_str);

    html += "<tr>";
    if (is_directory(file_path)) {
      html += "<td><a href=\"" + files[i] + "/\">" + files[i] + "/</a></td>";
      html += "<td>-</td>";
    } else {
      html += "<td><a href=\"" + files[i] + "\">" + files[i] + "</a></td>";
      html += "<td>" + size_str + "</td>";
    }
    html += "<td>" + mod_time_str + "</td>";
    html += "</tr>";
  }

  html += "</table></body></html>\r\n\r\n";
  _responseBody = html;
}

void RequestHandler::get(const std::string fullPath) {
  std::ifstream s(fullPath.c_str());
  _responseBody.clear();
  if (!s.is_open() && !s.good() && !_reqStatus._autoIndex) {
    _responseStatus = NOT_FOUND;
    return;
  }
  if (_reqStatus._autoIndex) {
    createHeaderResp();
    createAutoindex(fullPath);
    return;
  }
  std::stringstream buffer;
  buffer << s.rdbuf();
  _responseBody = buffer.str() + "\r\n\r\n";
  s.close();
  _responseStatus = OK;
  if (fullPath.find(".html") != std::string::npos)
    createHeaderResp();
  else
    createHeaderResp("media-type");
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
  createHeaderResp();
}

bool RequestHandler::deleteR() { return true; }

void RequestHandler::createResponse(Server *serv, int fd) {
  if (_responseStatus == OK) {
    if (_path.find("/cgi-bin/") != std::string::npos) {
      if (_path.find(".py") != std::string::npos)
        handleCgi(serv, fd, PYTHON);
      else if (_path.find(".go") != std::string::npos)
        handleCgi(serv, fd, GO);
      else if (_path.find(".html") != std::string::npos && _method == "GET") {
        get(serv->composedPath() + _path);
      } else
        _responseStatus = NOT_FOUND;
    } else if (_method == "GET") {
      if (_path.find(".") == std::string::npos) {
        if (_path[_path.length() - 1] != '/')
          _path.append("/");
        get(serv->composedPath() + _path + _reqStatus._indexPage);
      } else
        get(serv->composedPath() + _path);
    } else if (_method == "POST") {
      post(serv->composedPath() + _path + "/uploadedFiles/");
      _responseBody = "OK";
    }
  }
  if (_responseStatus <= 302) {
    std::cout << "REDIR => " << _reqStatus._redir << std::endl;
    if (_reqStatus._redir.length() > 0) {
      createHeaderResp("text/html", "Location:" + _reqStatus._redir + "\r\n");
      _responseBody = "BEING REDIRECTED";
    }
    std::cout << "Sending response" << std::endl;
    sendResp(fd);
  } else {
    error(fd, serv->getErrPage(_responseStatus));
  }
}

int waitpid_with_timeout(pid_t pid, int *status, int timeout) {
  int elapsed_time = 0;
  while (elapsed_time < timeout * 10000) { // Convert seconds to milliseconds
    int result = waitpid(pid, status, WNOHANG);
    if (result != 0) {
      return 0;
    }
    poll(NULL, 0, 100); // Sleep for 100 milliseconds
    elapsed_time += 100;
  }
  std::cout << "error: timeout. killing pid " << std::endl;
  return 1;
}

void RequestHandler::handleCgi(Server *serv, const int clientFd, int lang) {
  std::string Cmd;
  char *argv[4];
  std::string arg = (serv->composedPath() + _path);
  if (lang == 0) {
    if (arg.find("/upload.py") != std::string::npos)
      argv[2] = (char *)Utils::to_string(clientFd).c_str();
    else
      argv[2] = NULL;
    argv[1] = (char *)arg.c_str();
    Cmd = "/usr/bin/python3";
  } else {
    argv[1] = (char *)"run";
    argv[2] = (char *)arg.c_str();
    Cmd = "/usr/local/go/bin/go";
  }
  argv[0] = (char *)Cmd.c_str();
  argv[3] = NULL;

  if (access(("/" + arg).c_str(), R_OK | X_OK)) {
    int fd[2];
    if (pipe(fd) == -1)
      std::cout << "error opening pipe" << std::endl;
    int pid = fork();
    if (!pid) {
      close(fd[0]);
      std::cout << "starting exec" << std::endl;
      if (dup2(fd[1], 1) == -1 || close(fd[1]) == 1)
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
    while ((byteRead = read(fd[0], buffer, 1024 - 1)) > 0) {
      buffer[byteRead] = '\0';
      output += buffer;
    }
    close(fd[0]);
    _responseBody = Utils::to_string(buffer);
    createHeaderResp();
    sendResp(clientFd);
  } else
    _responseStatus = NOT_FOUND;
}

void RequestHandler::setBody(const std::string &body) { _body = body; }

void RequestHandler::createHeaderResp(const std::string contentType,
                                      const std::string optionalHeaders) {
  _responseHeader += "HTTP/1.1 " + Utils::to_string(_responseStatus) +
                     (_responseStatus == OK ? " OK" : " KO") +
                     "\r\n"
                     "Content-Type: " +
                     contentType +
                     "\r\n"
                     "Connection: close\r\n" +
                     optionalHeaders +
                     "\r\n"
                     "\r\n";
}

int RequestHandler::getResponseStatus() const { return _responseStatus; }

RequestHandler::~RequestHandler() {}
