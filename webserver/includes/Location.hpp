#pragma once

#include "ARules.hpp"

class Location : public ARules {
public:
  void printConf(const std::string &level) const;
  Location();
  Location(const Location &l);
  Location(const std::string &confTxt, std::vector<std::string> &parentMethods, int contentSize, const std::string &parentIndex);
  Location &operator=(const Location &l);
  // using ARules::getLocation;
  // using ARules::getMethods;
  ~Location();
};
