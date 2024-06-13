#include "../includes/Location.hpp"
#include <vector>

Location::Location() : ARules(1) {}

Location::Location(const Location &l) : ARules() { *this = l; }

Location::Location(const std::string &confTxt,
                   std::vector<std::string> &parentMethods, int parentClientSize, const std::string &parentIndex)
    : ARules(1) {
  std::istringstream iss(confTxt);
  std::string line;
  std::string key;
  while (!std::getline(iss, line).eof()) {
    if (!line.empty()) {
      std::istringstream lineStream(line);
      lineStream >> key;
      parseConf(key, lineStream, iss);
    }
  }
  if(_index.length() == 0)
    _index = parentIndex;
  if (_methods.empty())
    _methods = parentMethods;
  if(_clientSize == -1)
    _clientSize = parentClientSize;
}

Location &Location::operator=(const Location &l) {
  if (this == &l)
    return *this;
  _autoIndex = l._autoIndex;
  _root = l._root;
  _index = l._index;
  _return = l._return;
  _methods = l._methods;
  _location = l._location;
  _isLocation = l._isLocation;
  _clientSize = l._clientSize;
  return *this;
}

void Location::printConf(const std::string &level) const {
  std::cout << level << "autoindex: " << _autoIndex << std::endl;
  std::cout << level << "root: " << _root << std::endl;
  std::cout << level << "index: " << _index << std::endl;
  std::cout << level << "return: " << _return << std::endl;
  std::cout << level << "methods: " << std::endl;
  for (std::vector<std::string>::const_iterator it = _methods.begin();
       it != _methods.end(); it++)
    std::cout << level << level << *it << std::endl;
  for (std::map<std::string, ARules *>::const_iterator it = _location.begin();
       it != _location.end(); ++it) {
    std::cout << level << "location: " << it->first << std::endl;
    dynamic_cast<Location *>(it->second)->printConf(level + "\t");
  }
  std::cout << level << "isLocation: " << _isLocation << std::endl;
  std::cout << level << "clientSize: " << _clientSize << std::endl;
}

Location::~Location() {}
