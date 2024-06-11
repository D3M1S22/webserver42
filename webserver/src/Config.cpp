#include "../includes/Config.hpp"
#include <cstdio>
#include <fstream>
#include <sstream>

Config::Config() {}

Config::Config(const std::string &configFileName) {
  _configFile.open(configFileName.c_str());
}

Config &Config::operator=(const Config &c) {
  (void)c;
  return *this;
}

void Config::parseConfig() {
  std::string line;
  int serverFound = -1;
  int prevLine = 1;
  int lineCount = 1;
  std::string server = "";
  std::stringstream linCStr;
  int col;
  while (1) {
    col = 0;
    while (!getline(_configFile, line).eof()) {
      linCStr.str("");
      linCStr << lineCount;
      if (serverFound == -1 && line.substr(0, 6) == "server") {
        while (line[col + 6] == ' ' && line[col + 6] != 0)
          col++;
        if (line[col + 6] != '{')
          throw Error("ERROR NO SERVER FOUND CHECK ./config/basic.conf");
        serverFound++;
      } else if (line.substr(0, 6) == "server" && serverFound == 0 &&
                 lineCount != prevLine) {
        std::string err = "ERROR SERVER INSIDE SERVER ON LINE " + linCStr.str();
        throw Error(err.c_str());
      } else if (serverFound == -1)
        continue;
      if (line[0] == '}') {
        server += line + "\n";
        serverFound++;
        break;
      }
      server += line + "\n";
      lineCount++;
      prevLine = lineCount - 1;
    }
    if (serverFound == 0) {
      std::string err =
          "ERRORR MISSING CLOSING BRACKET } ON LINE " + linCStr.str();
      throw Error(err.c_str());
    }
    if (serverFound == 1) {
      Server s(server);
      _servers.push_back(s);
      server = "";
      serverFound = -1;
    }
    if (_configFile.eof())
      break;
  }
}

std::vector<Server> &Config::getServers() { return _servers; }

Config::~Config() {
  if (_configFile.is_open())
    _configFile.close();
}
