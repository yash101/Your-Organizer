#include "webserver.h"
#include "httprequesthandler.h"

void core::MainServer::requestHandler(http::HttpSession& session)
{
  yo::httpRequestHandler(session);
}
