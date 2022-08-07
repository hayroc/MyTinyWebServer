#include "webserver.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) {
  std::string ip = "127.0.0.1";
  int port = 3000;
  if(argc >= 2) {
    ip = std::string(argv[1]);
    port = std::atoi(argv[2]);
  }

  hayroc::WebServer server(ip, port);
  server.run();
}
