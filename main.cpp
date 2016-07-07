#include "base/config.h"
#include "core/webserver.h"
#include "core/httprequesthandler.h"

int main()
{
  conf::init();

  yo::HostHandler handler;
  handler.matcher = "(.*)";
  handler.matching_method = yo::mm_regex;
  yo::HttpAction action;
  action.function_pointer = yo::static_handler;
  action.data = NULL;
  action.matcher = "(.*)";
  action.matching_method = yo::mm_regex;
  handler.actions.insert(std::pair<size_t, yo::HttpAction>(0, action));

  yo::hostnames.insert(std::pair<size_t, yo::HostHandler>(0, handler));

  core::MainServer server;
  server.setPort(2000);
  server.setTcpQueueSize(5);
  server.setSocketTimeout(0, 10);
  server.setMaximumConcurrentClients(500);
  server.startServer();
}
