#pragma once

#include <arpa/inet.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <variant>
#include <vector>

namespace redis {

namespace data {

struct RString {
  enum class Type { Simple, Bulk, Verbatim } type;
  std::string value;
  // only used for verbatim strings
  std::string encoding;
};

struct RInteger {
  int64_t value;
};

struct RDouble {
  double value;
};

struct RBoolean {
  bool value;
};

struct RNull {
  static constexpr std::nullptr_t value = nullptr;
};

struct RBigInteger {
  std::string value;
};

struct RError {
  enum class Type { Simple, Bulk } type;
  std::string value;
};

struct RArray;
struct RMap;
struct RSet;
struct RPush;

using RValue = std::variant<RString, RError, RInteger, RBigInteger, RDouble,
                            RBoolean, RNull, RArray, RMap, RSet, RPush>;

struct RArray {
  std::vector<RValue> value;
};

struct RMap {
  std::vector<std::pair<RValue, RValue>> value;
};

struct RSet {
  std::vector<RValue> value;
};

struct RPush {
  std::vector<RValue> value;
};

struct RValueComparator {
  bool operator()(const RValue &lhs, const RValue &rhs) const;
};

} // namespace data

namespace error {
struct IOError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct EndOfStream : public IOError {
  using IOError::IOError;
};

struct Invalid : public std::runtime_error {
  using std::runtime_error::runtime_error;
};
}; // namespace error

} // namespace redis