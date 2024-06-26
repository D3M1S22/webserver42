#include "../includes/ARules.hpp"
#include "../includes/Location.hpp"
#include <cstddef>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

ARules::ARules() {}

ARules::ARules(const int &isLocation)
    : _root("/default"), _index(""), _autoIndex(false),
      _isLocation(isLocation) {
  if (!isLocation) {
    _methods.push_back("GET");
    _methods.push_back("POST");
    _methods.push_back("DELETE");
    _clientSize = 2048;
  } else {
    _clientSize = -1;
  }
}

ARules::~ARules() {}

void ARules::parseConf(const std::string &key, std::istringstream &s,
                       std::istringstream &conf) {
  std::string v;
  std::string line;
  std::string locationConf;
  if (key == "index") {
    std::string index;
    s >> _index;
  } else if (key == "root") {
    std::string root;
    s >> root;
    _root = root;
  } else if (key == "return") {
    s >> _return;
  } else if (key == "methods") {
    _methods.clear();
    std::string method;
    while (s >> method) {
      _methods.push_back(method);
    }
  } else if (key == "client_size") {
    s >> _clientSize;
  } else if (key == "redirect") {
    s >> _redirection;
  } else if (key == "autoindex") {
    std::string value;
    s >> value;
    _autoIndex = (value == "on");
  } else if (key == "location") {
    while (std::getline(conf, line)) {
      if (line[4] == '}')
        break;
      locationConf += line + "\n";
    }
    std::string locationName;
    s >> locationName;
    _location[locationName] = new Location(locationConf, _methods, _clientSize);
  }
}

ARules::ARules(const ARules &ar) { *this = ar; }

ARules &ARules::operator=(const ARules &ar) {
  if (this == &ar)
    return *this;
  _autoIndex = ar._autoIndex;
  _root = ar._index;
  _return = ar._return;
  _methods = ar._methods;
  _location = ar._location;
  _index = ar._index;
  _clientSize = ar._clientSize;
  _isLocation = ar._isLocation;
  _redirection = ar._redirection;
  return *this;
}

std::vector<std::string> &ARules::getMethods() { return _methods; }
std::map<std::string, ARules *> &ARules::getLocation() { return _location; }
std::string ARules::getRoot() const { return _root; }

int ARules::compareMethod(const std::string &method, const int contentSize) {
  int val = 0;
  if (std::find(_methods.begin(), _methods.end(), method) != _methods.end())
    val = 1;
  if (contentSize <= _clientSize) {
    val = val | 2;
  } else {
    val = val | 8;
  }
  return val;
}

// New function to check if a method is allowed for a given path
void ARules::isAllowed(std::string path, const std::string &method,
                       const int contentSize, reqStatusParams &rqParams) {
  std::map<std::string, ARules *>::iterator it = _location.begin();
  for (; it != _location.end(); it++) {
    if (!std::strncmp(it->first.c_str(), path.c_str(), it->first.length())) {
      std::cout << "path on allowed " << path << std::endl;
      path = path.erase(0, it->first.length());
      std::cout << it->first << " redirection " << it->second->_redirection
                << std::endl;
      if (path.length() == 0 || (path.length() == 1 && path[0] == '/')) {
        rqParams._allACs |= it->second->compareMethod(method, contentSize);
        rqParams._indexPage = it->second->_index;
        rqParams._redir = it->second->_redirection;
        if (it->second->_autoIndex)
          rqParams._autoIndex = true;
        return;
      } else {
        it->second->isAllowed(path, method, contentSize, rqParams);
        return;
      }
    }
  }
  rqParams._allACs |= compareMethod(method, contentSize);
  rqParams._indexPage = _index;
}

std::string ARules::getLocationIndex(std::string s) {
  std::map<std::string, ARules *>::iterator it = _location.begin();
  for (; it != _location.end(); it++) {
    if (!std::strncmp(it->first.c_str(), s.c_str(), it->first.length())) {
      s = s.erase(0, it->first.length());
      if (s.length() == 0) {
        return it->second->_index;
      } else
        return it->second->getLocationIndex(s);
    }
  }
  return _index;
}
