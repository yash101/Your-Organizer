#include "httpserver.h"
#include "exceptions.h"
#include "config.h"

#include <boost/algorithm/string.hpp>

#include <sstream>

#define me (*this)

static size_t maxlen_firstline;
static bool isInit = false;
static void init()
{
  conf::getConfigInt("http request firstline maximumlength");
}

void http::HttpServer::start()
{
  if(!isInit) init();
  boost::asio::ip::tcp::acceptor acceptor(me.ioService, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v6(), me.listeningPort));
  while(true)
  {
    boost::shared_ptr<http::Socket> connection(new http::Socket(me.ioService));
    acceptor.accept(*connection);
    boost::thread thread(boost::bind(&http::HttpServer::runner, this, connection));
  }
}

void http::HttpServer::runner(boost::shared_ptr<http::Socket> connection)
{
  try
  {
    me.parseHttpRequest(connection);
  }
  catch(std::exception& e)
  {}
}

void http::HttpServer::parseHttpRequest(boost::shared_ptr<http::Socket> socket)
{
  http::HttpSession session;
  session.socket = socket;

  //We do not support SSL or compression yet!
  session.information ^= http::SslEnabled | http::CompressionEnabled;

  me.processRequest(session);
}

void http::HttpServer::processRequest(http::HttpSession& session)
{
  //Read the first line
  std::string buf0 = me.readLine(session, maxlen_firstline);
  std::vector<std::string> parts;
  boost::split(parts, buf0, boost::is_any_of(" "));
  if(bpath.size() != 3)
  {
    throw EXCEPTION("Error! Unable to process request. Initial request line invalid!", buf0.size());
  }
}

std::string http::HttpServer::readLine(http::HttpSession& session, size_t len)
{
  std::stringstream str;
  char och = 'x';
  size_t ret = 0;
  while(true)
  {
    char ch;
    session.socket->read_some(boost::asio::buffer(&ch, sizeof(char)));
    if((och == '\r' || och == '\n') && (ch == '\n'))
      break;
    str << ch;
    ret++;
    if(len != 0 && ret >= len)
      throw EXCEPTION("Maximum supported bytes were read!", ret);
    och = ch;
  }
  return str.str();
}

/* HttpSession functions */

const std::string http::HttpSession::getPath()
{
  return me.path;
}

const std::string http::HttpSession::getUnprocessedPath()
{
  return me.path_unprocessed;
}

const http::DataSource http::HttpSession::getQuery(std::string key)
{
  if(me.get_queries.count(key) > 0)
    return me.get_queries[key];
  else
    return me.post_queries[key];
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
