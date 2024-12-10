#pragma once

#include <algorithm>
#include <arpa/inet.h>
#include <array>
#include <concepts>
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

#include "data.h"

namespace redis {

namespace resp {

using namespace data;
using namespace error;

template <size_t N> //
struct CharBuffer {
  static constexpr size_t capacity = N;
  std::array<char, N> bytes{};

  size_t buffer_first{}, buffer_last{};

  auto size() const { return buffer_last - buffer_first; }

  bool empty() const { return buffer_first == buffer_last; }
  void clear() { buffer_first = buffer_last = 0; }

  void consume(size_t n) { buffer_first += n; }

  void extend(size_t n) { buffer_last += n; }
};

template <typename T>
concept RawReader = requires(T t, char *buffer, size_t size) {
  { t(buffer, size) } -> std::same_as<ssize_t>;
};

template <RawReader R> //
struct BufferedReader {
  R reader;
  CharBuffer<1024> buffer{};

  RString parseSimpleString() {
    return RString{
        .type = RString::Type::Simple,
        .value = nextSimpleEncodedChars(),
    };
  }

  RError parseSimpleError() {
    return RError{
        .type = RError::Type::Simple,
        .value = nextSimpleEncodedChars(),
    };
  }

  RInteger parseInteger() {
    return RInteger{
        .value = nextInteger(),
    };
  }

  RString parseBulkString() {
    return RString{
        .type = RString::Type::Bulk,
        .value = nextBulkEncodedChars(),
    };
  }

  RArray parseArray() {
    int64_t length = nextInteger();
    if (length < 0) {
      throw Invalid("Invalid RESP array");
    }
    RArray array{};
    array.value.reserve(length);
    for (int64_t i = 0; i < length; i++) {
      array.value.emplace_back(readValue());
    }
    return array;
  }

  RNull parseNull() {
    if (nextChar() != '\r' || nextChar() != '\n') {
      throw Invalid("Invalid RESP null");
    }
    return RNull{};
  }

  RBoolean parseBoolean() {
    char c = nextChar();

    bool value;
    if (c == 't') {
      value = true;
    } else if (c == 'f') {
      value = false;
    } else {
      throw Invalid("Invalid RESP boolean");
    }

    if (nextChar() != '\r' || nextChar() != '\n') {
      throw Invalid("Invalid RESP boolean");
    }

    return RBoolean{
        .value = value,
    };
  }

  RDouble parseDouble() {
    std::string chars = nextSimpleEncodedChars();
    try {
      return RDouble{
          .value = std::stod(chars),
      };
    } catch (const std::logic_error &e) {
      throw Invalid("Invalid RESP double");
    }
  }

  RBigInteger parseBigInteger() {
    return RBigInteger{
        .value = nextSimpleEncodedChars(),
    };
  }

  RError parseBulkError() {
    return RError{
        .type = RError::Type::Bulk,
        .value = nextBulkEncodedChars(),
    };
  }

  RString parseVerbatismString() {
    int64_t length = nextInteger();
    if (length < 4) {
      throw Invalid("Invalid RESP verbatim string");
    }

    std::string encoding(3, 0);

    nextChars(encoding.data(), 3);
    if (nextChar() != ':') {
      throw Invalid("Invalid RESP verbatim string");
    }

    std::string chars(length - 4, 0);

    nextChars(chars.data(), length - 4);
    if (nextChar() != '\r' || nextChar() != '\n') {
      throw Invalid("Invalid RESP verbatim string");
    }

    return RString{
        .type = RString::Type::Verbatim,
        .value = std::move(chars),
        .encoding = std::move(encoding),
    };
  }

  RMap parseMap() {
    int64_t length = nextInteger();
    if (length < 0) {
      throw Invalid("Invalid RESP map");
    }
    RMap map{};
    for (int64_t i = 0; i < length; i++) {
      RValue key = readValue();
      RValue value = readValue();
      map.value.emplace_back(std::move(key), std::move(value));
    }
    return map;
  }

  RSet parseSet() {
    int64_t length = nextInteger();
    if (length < 0) {
      throw Invalid("Invalid RESP set");
    }

    RSet set{};

    set.value.reserve(length);

    for (int64_t i = 0; i < length; i++) {
      set.value.emplace_back(readValue());
    }

    return set;
  }

  RPush parsePush() {
    int64_t length = nextInteger();

    if (length < 0) {
      throw Invalid("Invalid RESP push");
    }

    RPush push{};

    push.value.reserve(length);

    for (int64_t i = 0; i < length; i++) {
      push.value.emplace_back(readValue());
    }

    return push;
  }

  RValue readValue() {
    char type = nextChar();
    switch (type) {
    case '+':
      return parseSimpleString();
    case '-':
      return parseSimpleError();
    case ':':
      return parseInteger();
    case '$':
      return parseBulkString();
    case '*':
      if (peekChar() == '-') {
        nextInteger();
        return RNull{};
      }
      return parseArray();
    case '_':
      return parseNull();
    case '#':
      return parseBoolean();
    case ',':
      return parseDouble();
    case '(':
      return parseBigInteger();
    case '!':
      return parseBulkError();
    case '%':
      return parseMap();
    case '~':
      return parseSet();
    case '>':
      return parsePush();
    default:
      throw Invalid("Invalid RESP object type");
    }
    // unreachable
    __builtin_unreachable();
  }

private:
  int64_t nextInteger() {
    int64_t value{};
    char c;

    while ((c = nextChar()) != '\r') {
      value = value * 10 + (c - '0');
    }

    if (nextChar() != '\n') {
      throw Invalid("Invalid RESP integer");
    }

    return value;
  }

  std::string nextSimpleEncodedChars() {
    std::string value;
    char c;

    while ((c = nextChar()) != '\r') {
      value.push_back(c);
    }

    if (nextChar() != '\n') {
      throw Invalid("Invalid RESP simple string");
    }

    return value;
  }

  std::string nextBulkEncodedChars() {
    int64_t length = nextInteger();

    if (length < 0) {
      throw Invalid("Invalid RESP bulk string");
    }

    std::string value;
    value.resize(length);

    nextChars(value.data(), length);
    if (nextChar() != '\r' || nextChar() != '\n') {
      throw Invalid("Invalid RESP bulk string");
    }

    return value;
  }

  char nextChar() {
    if (buffer.empty()) {
      readMore();
    }

    return buffer.bytes[buffer.buffer_first++];
  }

  void nextChars(char *out, size_t n) {
    size_t off{};

    while (off < n) {
      if (buffer.empty()) {
        readMore();
      }
      size_t to_copy = std::min(n - off, buffer.size());

      std::copy_n(buffer.bytes.data() + buffer.buffer_first, to_copy,
                  out + off);

      buffer.consume(to_copy);

      off += to_copy;
    }
  }

  char peekChar() {
    if (buffer.empty()) {
      readMore();
    }

    return buffer.bytes[buffer.buffer_first];
  }

  void readMore() {
    ssize_t bytes_read = reader(buffer.bytes.data() + buffer.buffer_last,
                                buffer.capacity - buffer.buffer_last);

    if (bytes_read < 0) {
      throw IOError("Failed to read more bytes");
    } else if (bytes_read == 0) {
      throw EndOfStream("End of stream reached");
    }

    buffer.buffer_last += bytes_read;
  }
};

template <typename T>
concept RawWriter = requires(T t, const char *buffer, size_t size) {
  { t(buffer, size) } -> std::same_as<ssize_t>;
};

template <RawWriter W> //
struct BufferedWriter {

  W writer;
  CharBuffer<1024> buffer{};

  void flush() {
    while (buffer.size() > 0) {
      ssize_t bytes_written =
          writer(buffer.bytes.data() + buffer.buffer_first, buffer.size());

      if (bytes_written < 0) {
        throw IOError("Failed to write bytes");
      } else if (bytes_written == 0) {
        throw EndOfStream("End of stream reached");
      }

      buffer.consume(bytes_written);
    }
  }

  void print(std::string_view bytes) {
    size_t off{};

    while (off < bytes.size()) {
      if (buffer.size() == buffer.capacity) {
        flush();
      }
      size_t to_copy =
          std::min(bytes.size() - off, buffer.capacity - buffer.buffer_last);
      std::copy_n(bytes.data() + off, to_copy,
                  buffer.bytes.data() + buffer.buffer_last);
      buffer.extend(to_copy);
      off += to_copy;
    }
  }

  void print(std::integral auto value) { print(std::to_string(value)); }
  void print(std::floating_point auto value) { print(std::to_string(value)); }

  template <typename T1, typename T2, typename... Args>
  void print(T1 &&t1, T2 &&t2, Args &&...args) {
    print(std::forward<T1>(t1));
    print(std::forward<T2>(t2), std::forward<Args>(args)...);
  }

  void writeValue(const RString &value) {
    using namespace std::string_view_literals;

    if (value.type == RString::Type::Simple) {
      print("+"sv, value.value, "\r\n"sv);
    } else {
      if (value.encoding.empty()) {
        print("$"sv, value.value.size(), "\r\n"sv, value.value, "\r\n"sv);
      } else {
        print("="sv, value.value.size() + 4, value.encoding, ":"sv, value.value,
              "\r\n"sv);
      }
    }
  }

  void writeValue(const RError &value) {
    using namespace std::string_view_literals;

    if (value.type == RError::Type::Simple) {
      print("-"sv, value.value, "\r\n"sv);
    } else {
      print("!"sv, value.value.size(), "\r\n"sv, value.value, "\r\n"sv);
    }
  }

  void writeValue(const RInteger &value) {
    using namespace std::string_view_literals;
    print(":"sv, value.value, "\r\n"sv);
  }

  void writeValue(const RArray &value) {
    using namespace std::string_view_literals;

    print("*"sv, value.value.size(), "\r\n"sv);
    for (const auto &v : value.value) {
      writeValue(v);
    }
  }

  void writeValue(const RNull &value) {
    using namespace std::string_view_literals;

    print("_\r\n"sv);
  }

  void writeValue(const RBoolean &value) {
    using namespace std::string_view_literals;

    print("#"sv, value.value ? "t"sv : "f"sv, "\r\n"sv);
  }

  void writeValue(const RDouble &value) {
    using namespace std::string_view_literals;

    print(","sv, value.value, "\r\n"sv);
  }

  void writeValue(const RBigInteger &value) {
    using namespace std::string_view_literals;

    print("("sv, value.value, "\r\n"sv);
  }

  void writeValue(const RMap &value) {
    using namespace std::string_view_literals;

    print("%"sv, value.value.size(), "\r\n"sv);
    for (const auto &[k, v] : value.value) {
      writeValue(k);
      writeValue(v);
    }
  }

  void writeValue(const RSet &value) {
    using namespace std::string_view_literals;

    print("~"sv, value.value.size(), "\r\n"sv);
    for (const auto &v : value.value) {
      writeValue(v);
    }
  }

  void writeValue(const RPush &value) {
    using namespace std::string_view_literals;

    print(">"sv, value.value.size(), "\r\n"sv);
    for (const auto &v : value.value) {
      writeValue(v);
    }
  }

  void writeValue(const RValue &value) {
    std::visit([&](const auto &v) { writeValue(v); }, value);
  }
};
} // namespace resp
} // namespace redis
