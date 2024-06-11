#include "./includes/Config.hpp"
#include <sys/wait.h>

bool checkInput(int argc, std::string path, std::string &errorMsg) {
  switch (argc) {
  case 1:
    errorMsg = "Missing config file as input";
    return true;
    break;
  case 2:
    std::ifstream file(path.c_str());
    if (file.good())
      return false;
    else {
      errorMsg = "Error with the file " + path;
      return true;
    }
  }
  return false;
}

int main(int argc, char **argv) {
  std::string errorMsg = "";
  if (checkInput(argc, argc >= 2 ? argv[1] : "", errorMsg)) {
    std::cout << errorMsg << std::endl;
    return 1;
  }
  Config c(argv[1]);
  try {
    c.parseConfig();
  } catch (const Error &e) {
    std::cerr << e.message() << std::endl;
    return 1;
  }
  std::vector<Server>::iterator it = c.getServers().begin();
  std::vector<Server>::iterator ite = c.getServers().end();
  int pid;
  for (; it != ite; it++) {
    pid = fork();
    if (!pid)
      it->start();
  }
  waitpid(pid, NULL, 0);
  return 0;
}
