#include "HttpServer.hpp"

int main() {
  TinyNet::Reactor reactor;
  TinyNet::IPv4Address listenAddr("0.0.0.0", 8888);
  TinyNet::HttpServer httpServer(&reactor, listenAddr, 8);
  httpServer.start();
}