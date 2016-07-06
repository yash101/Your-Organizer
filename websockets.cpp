#include "httpserver.h"
#include "http_internals.h"
using namespace http;
#define me (*this)

int HttpServer::initializeWebsockets(HttpSession& session, WebsocketsSession& wsession)
{
  //Set the connection variables
  wsession.connection = session.connection;
  wsession.httpSession = &session;
  wsession.websocketKey = session.incoming_headers["sec-websocket-key"];
  if(wsession.websocketKey.size() == 0) throw HTTPEXCEPT("Invalid WebSocket request! No \"Sec-Websocket-Key\" header present", 400, 400);

  //Run the user initialization function (before the handshake)
  me.websocketInit(session);

  //Complete the handshake and establish connection
}

int HttpServer::websocketHandshake(HttpSession& session, WebsocketsSession& wsession)
{
  //Send the first few headers
  session.connection->write("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: upgrade\r\n");
}


void HttpServer::websocketInit(HttpSession& session)
{
}

void HttpServer::websocketWorker(WebsocketsSession& session)
{
}
