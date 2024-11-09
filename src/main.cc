#include "server.h"

int main(int argc, char **argv) {
  redis::server::Server server;

  server.start();

  return 0;
}
