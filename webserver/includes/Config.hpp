#pragma once

#include "Error.hpp"
#include "Server.hpp"
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <vector>

class Config {
private:
  std::ifstream _configFile;
  Config();
  Config &operator=(const Config &);
  Config(const Config &);
  std::vector<Server> _servers;

public:
  Config(const std::string &configFileName);
  void parseConfig();
  std::vector<Server> &getServers();
  ~Config();
};
