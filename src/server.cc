#include <cstdlib>
#include <iostream>
#include <thread>
#include <mutex>

#include "utils.h"
#include "data.h"
#include "resp.h"
#include "server.h"

namespace redis {
namespace server {
Server::Server() {
  // Create a socket
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    throw "Failed to create server socket\n";
  }

  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) <
      0) {
    throw "setsockopt failed\n";
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = parse_addr(host);
  server_addr.sin_port = htons(port);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) !=
      0) {
    throw "Failed to bind to port 6379\n";
  }
}

Server::~Server() { close(server_fd); }

void Server::start() {
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    throw "listen failed\n";
  }

  std::cout << "Redis Server is listening on " << host << ":" << port
            << std::endl;

  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);

  // handle client requests
  while (true) {
    // Accept a connection from a client
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
                           (socklen_t *)&client_addr_len);
    if (client_fd < 0) {
      std::cerr << "Failed to accept " << client_fd << std::endl;
    }

    std::cout << "Client " << client_fd << " connected\n";

    // start new thread to handle request.
    std::thread th(&Server::handle_request, this, client_fd);
    th.detach();
  }
}

void Server::handle_request(int client_fd) {
  using namespace redis::data;
  using namespace redis::resp;

  BufferedReader reader{
      .reader =
          [client_fd](char *buffer, size_t size) {
            return read(client_fd, buffer, size);
          },
  };

  BufferedWriter _writer{
      .writer =
          [client_fd](const char *buffer, size_t size) {
            return write(client_fd, buffer, size);
          },
  };

  BufferedWriter printer{
      .writer = [](const char *buffer, size_t size) -> ssize_t {
        if (fwrite(buffer, 1, size, stdout) < 0) {
          return -1;
        }
        return size;
      },
  };

  std::recursive_mutex mutex;
  auto reply = [&](const RValue &value) {
    std::lock_guard lock(mutex);
    _writer.writeValue(value);
    _writer.flush();
  };

  while (true) {
    try {
      // reads a command from client
      RValue value = reader.readValue();
      RArray &command = std::get<RArray>(value);
      std::vector<std::string> commandLine;
      for (const auto &v : command.value) {
        commandLine.emplace_back(std::get<RString>(v).value);
      }

      // prints the command
      std::cout << "Command: ";
      for (const auto &c : commandLine) {
        std::cout << c << ' ';
      }

      std::cout << '\n';
      if (commandLine.empty()) {
        reply(RError{
            .type = RError::Type::Simple,
            .value = "ERR Empty command",
        });
        continue;
      }

      // handles the command
      using namespace std::string_view_literals;
      std::string commandName = commandLine[0];
      std::transform(commandName.begin(), commandName.end(),
                     commandName.begin(), ::toupper);
      if (commandName == "COMMAND"sv) {
        reply(RArray{
            .value =
                {
                    RString{
                        .type = RString::Type::Simple,
                        .value = "COMMAND",
                    },
                    RString{
                        .type = RString::Type::Simple,
                        .value = "PING",
                    },
                    RString{
                        .type = RString::Type::Simple,
                        .value = "ECHO",
                    },
                },
        });
      } else if (commandName == "PING"sv) {
        if (commandLine.size() == 1) {
          reply(RString{
              .type = RString::Type::Simple,
              .value = "PONG",
          });
        } else {
          reply(RString{
              .type = RString::Type::Bulk,
              .value = commandLine[1],
          });
        }
      } else if (commandName == "ECHO"sv) {
        if (commandLine.size() == 1) {
          reply(RError{
              .type = RError::Type::Simple,
              .value = "ERR ECHO requires an argument",
          });
        } else {
          reply(RString{
              .type = RString::Type::Bulk,
              .value = commandLine[1],
          });
        }
      } else {
        reply(RError{
            .type = RError::Type::Simple,
            .value = "ERR Unknown command",
        });
      }
    } catch (const IOError &e) {
      break;
    } catch (const Invalid &e) {
      reply(RError{
          .type = RError::Type::Bulk,
          .value = std::string("ERR ") + e.what(),
      });
      continue;
    } catch (const std::bad_variant_access &e) {
      reply(RError{
          .type = RError::Type::Simple,
          .value = std::string("ERR Unexpected Redis type"),
      });
      continue;
    }
  }

  close(client_fd);
}

// parse addr_str: 127.0.0.1
in_addr_t Server::parse_addr(std::string addr_str) {
  auto parts = utils::split(addr_str, '.');
  if (parts.size() != 4)
    throw "Parse addr failed.";

  unsigned int result = 0;
  for (auto &it : parts) {
    result = (result << 8) + stoi(it);
  }

  return (in_addr_t)result;
}

} // namespace server
} // namespace redis
