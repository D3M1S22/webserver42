#pragma once

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

class ARules {
protected:
  std::string _root;
  std::string _index;
  std::string _return;
  std::vector<std::string> _methods;
  int _clientSize;
  bool _autoIndex;
  std::map<std::string, ARules *> _location;
  bool _isLocation;

public:
  ARules();
  ARules(const ARules &ar);
  ARules(const std::string &root, const std::string &index,
         const std::string &return_, const std::vector<std::string> &methods,
         const int &clientSize, const bool &autoIndex,
         const std::map<std::string, ARules *> &location,
         const bool &isLocation);
  ARules &operator=(const ARules &ar);
  ARules(const int &isLocation);
  void parseConf(const std::string &key, std::istringstream &s,
                 std::istringstream &conf);
  std::map<std::string, ARules *> &getLocation();
  std::vector<std::string> &getMethods();
  std::string getRoot() const;
  virtual void printConf(const std::string &level) const = 0;
  int isAllowed(std::string method, const std::string &path, int contentSize);
  int compareMethod(const std::string &method, int contentSize);\
  std::string getLocationIndex(std::string s);
  virtual ~ARules();
};
