#pragma once

#include <string>
#include <vector>

namespace redis {
namespace utils {
std::vector<std::string> split(const std::string &s, char delim);
} // namespace utils
} // namespace redis