#include "tcpserver.h"
#include "config.h"
#include <postgresql/libpq-fe.h>
#include <iostream>
#include "httpserver.h"

int main()
{
  conf::init();
  http::HttpServer server;
  server.setPort(8000);
  server.start();
}
