#include "httpserver.h"
#include "exceptions.h"
#define me (*this)

void http::HttpServer::start()
{
}

void http::HttpServer::runner()
{
}

void http::HttpServer::parseHttpRequest(boost::asio::ip::tcp::socket& socket)
{
  socket.write_some(boost::asio::buffer("Hello World!\n"));
}


/* Class basic functions/setup functions */

http::HttpServer::HttpServer() :
  listeningPort(0),
  isServerRunning(false)
{}

http::HttpServer::~HttpServer()
{}

//Less writing. Don't want to get arthritis :)
#define ERRR if(me.isServerRunning) {throw EXCEPTION("Error: Server is already running! May not change value!", -1);}
#define SET(var, val) if(!me.safeMode)return var=val;else {var=val; return "";}
#define GET(var) if(me.safeMode)return ""; else return var;
short http::HttpServer::getPort()
{
  return me.listeningPort;
}

short http::HttpServer::setPort(short port)
{
  ERRR
  return (me.listeningPort = port);
}
