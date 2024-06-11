#pragma once
#include <exception>
#include <iostream>

class Error {
public:
  Error(const std::string &message);

  const std::string &message() const;

private:
  std::string message_;
};
