#include "../includes/Error.hpp"

Error::Error(const std::string &message) : message_(message) {}

const std::string &Error::message() const { return message_; }
