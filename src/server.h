#pragma once

#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

namespace redis {
namespace server {
class Server {
public:
  Server();
  ~Server();
  void start();
  void handle_request(int client_fd);

private:
  int server_fd;
  std::string host{"0.0.0.0"}; // host ip v4 like: 127.0.0.1
  unsigned short port{6379};

  // addr_str: 127.0.0.1
  in_addr_t parse_addr(std::string addr_str);
};
} // namespace server
} // namespace redis