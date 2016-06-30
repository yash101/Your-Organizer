#include "tcpsockets.h"
#include <thread>

int main(int argc, char** argv)
{
  sock::initialize();
  sock::SocketServer server;
  server.addListeningPort(8081);
  server.addListeningPort(8080);
//  sock::SSLArguments args("privkey.pem");
//  server.addListeningPort(8082);
  std::thread([&](){
    std::this_thread::sleep_for(std::chrono::seconds(5));
    server.stopServer();
  }).detach();
  server.startServer();
  sock::deinitialize();
  return 0;
}
