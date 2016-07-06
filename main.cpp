#include "base/config.h"
#include "server/httpserver.h"

int main()
{
  http::HttpServer server;
  server.setPort(2001);
  server.startServer();
}
