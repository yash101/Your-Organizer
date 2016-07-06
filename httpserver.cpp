#include "httpserver.h"
#include "http_internals.h"
#include "exceptions.h"
#include "config.h"
#include "string_algorithms.h"

#include <boost/algorithm/string.hpp>

#include <sstream>
#include <time.h>
#include <stdio.h>

#define me (*this)
using namespace http;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Configuration variables */
namespace vars
{
  static size_t maxlen_firstline;	//Maximum number of bytes downloaded in the first line ({METHOD} {URI} HTTP/{v}\r\n)
  static size_t max_ngetq;				//Maximum number of HTTP GET queries
  static size_t max_hdrct;				//Maximum number of HTTP headers to download
  static size_t max_hdrlen;
  static size_t max_hdrklen;			//Maximum length of a header key ({key}: {value}\r\n)
  static size_t max_hdrvlen;			//Maximum length of a header value ({key}: {value}\r\n)
  static size_t max_postlen;			//Maximum data to download from POST
  static size_t max_postdata; 		//Maximum number of bytes to download from client
  static size_t max_postkeylen;  	//Maximum key length for a POST query
  static size_t max_ncookies;			//Maximum number of cookies downloaded
  static size_t max_cookiekeylen;	//Maximum length of a cookie key
  static size_t max_cookievallen;	//Maximum length of a cookie's value

  static bool isInit = false;

  static void init()
  {
    if(isInit) return;
    isInit = true;

    //Set up configuration variables
    maxlen_firstline = conf::getConfigInt("http request request_line maximum_length");
    max_ngetq = conf::getConfigInt("http request queries get maximum_count");
    max_hdrct = conf::getConfigInt("http request headers maximum_count");
    max_hdrlen = conf::getConfigInt("http request headers maximum_length");
    max_hdrklen = conf::getConfigInt("http request headers maximum_key_length");
    max_hdrvlen = conf::getConfigInt("http request headers maximum_value_length");
    max_postlen = conf::getConfigInt("http request queries post maximum_count");
    max_postdata = conf::getConfigInt("http request queries post maximum_data_usage");
    max_postkeylen = conf::getConfigInt("http request queries post maximum_key_length");
    max_ncookies = conf::getConfigInt("http request cookies maximum_count");
    max_cookiekeylen = conf::getConfigInt("http request cookies maximum_key_length");
    max_cookievallen = conf::getConfigInt("http request cookies maximum_value_length");
  }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Server functions */

HttpServer::HttpServer()
{
  vars::init();
}

void HttpServer::worker(srv::TcpServerConnection& connection)
{
  HttpSession session;
  session.information = 0;
  session.connection = &connection;
  int ret = 0;

  ret = me.processRequestLine(session);
  ret = me.processHeaders(session);
  ret = me.processPostQueries(session);

  ret = me.checkRequest(session);
  ret = me.prepareSession(session);

  me.requestHandler(session);

  ret = me.checkResponse(session);
  ret = me.sendResponse(session);

  if(ret < 0) return;
}

int HttpServer::processRequestLine(HttpSession& session)
{
  std::string fline = session.connection->readline('\n', vars::maxlen_firstline);
  if(fline.back() == '\n' || fline.back() == '\r') fline.pop_back();
  //Should never get called unless there's an internal issue
  if(fline.back() == '\n' || fline.back() == '\r') fline.pop_back();

  std::vector<std::string> parts;
  boost::trim(fline);
  boost::split(parts, fline, boost::is_any_of(" "));

  session.information |= http::getRequestType(parts[0]);
  session.information |= http::getRequestProtocol(parts[2]);
  session.path_unprocessed = parts[1];

  me.processRequestUrl(session);
  return session.information;
}

void HttpServer::processRequestUrl(HttpSession& session)
{
  std::vector<std::string> parts = base::splitByFirstDelimiter(session.path_unprocessed, "?");
  if(parts.size() == 1) session.path = parts[0];
  else
  {
    session.path = parts[0];
    std::vector<std::string> queries;
    boost::split(queries, parts[1], boost::is_any_of("&"));
    if(queries.size() > vars::max_ngetq || vars::max_ngetq == 0)
    for(size_t i = 0; i < queries.size(); i++)
    {
      std::vector<std::string> qp = base::splitByFirstDelimiter(queries[i], "=");
      if(qp.size() == 1)
      {
        if(http::decodeURI(qp[0]).size() == 0) continue;
        else
        {
          session.get_queries[http::decodeURI(qp[0])].data = "";
          session.get_queries[http::decodeURI(qp[0])].type = DataSource::String;
        }
      }
      else
      {
        if(http::decodeURI(qp[0]).size() == 0) continue;
        else
        {
          session.get_queries[http::decodeURI(qp[0])].data = http::decodeURI(qp[1]);
          session.get_queries[http::decodeURI(qp[0])].type = DataSource::String;
        }
      }
    }
  }
}

int HttpServer::processHeaders(HttpSession& session)
{
  size_t i = 0;
  while(vars::max_hdrct == 0 || i++ < vars::max_hdrct)
  {
    std::string header = session.connection->readline('\n', vars::max_hdrlen);
    if(header.back() == '\n' || header.back() == '\r') header.pop_back();
    //Should never get called unless there's an internal issue
    if(header.back() == '\n' || header.back() == '\r') header.pop_back();

    std::vector<std::string> parts = base::splitByFirstDelimiter(header, ":");
    if(parts.size() != 0) boost::to_lower(parts[0]);

    if(parts.size() == 0)
    {
      break;
    }
    else if(parts.size() == 1)
    {
      if(parts[0] == "cookie") continue;
      session.incoming_headers[parts[0]] = "";
    }
    else
    {
      boost::trim(parts[1]);
      session.incoming_headers[parts[0]] = parts[1];
    }
  }

  //Check if we have a WebSockets connection request
  if(!strcasecmp(session.incoming_headers["connection"].c_str(), "upgrade"))
  {
    if(!strcasecmp(session.incoming_headers["upgrade"].c_str(), "websocket"))
    {
      return Websockets;
    }
  }

  return 0;
}

int HttpServer::processPostQueries(HttpSession& session)
{
  if(!(session.information & PostRequest)) return 0;
  if(session.incoming_headers["content-length"].size() == 0)
    throw HTTPEXCEPT("Error. No body \"Content-Length\" header provided!", 411, 411);
  size_t len = atoll(session.incoming_headers["content-length"].c_str());
  if(len > vars::max_postdata)
    throw HTTPEXCEPT("Error. Entity too large!", 413, 413);

  std::string str;
  session.connection->read(str, len);
  std::vector<std::string> queries;
  boost::split(queries, str, boost::is_any_of("&"));
  for(size_t i = 0; i < queries.size(); i++)
  {
    std::vector<std::string> qp = base::splitByFirstDelimiter(queries[i], "=");
    if(qp.size() == 1)
    {
      boost::trim(qp[0]);
      if(qp[0].size() == 0) continue;
      qp[0] = decodeURI(qp[0]);
      session.post_queries[qp[0]].data = "";
      session.post_queries[qp[0]].type = DataSource::String;
    }
    else if(qp.size() == 2)
    {
      boost::trim(qp[0]);
      if(qp[0].size() == 0) continue;
      boost::trim(qp[1]);
      qp[0] = decodeURI(qp[0]);
      qp[1] = decodeURI(qp[1]);
      session.post_queries[qp[0]].data = qp[1];
      session.post_queries[qp[0]].type = DataSource::String;
    }
  }

  return 0;
}

int HttpServer::checkRequest(HttpSession& session)
{
  if(session.information & Http1_1 || session.information & Http2_0)
  {
    if(session.incoming_headers.find("host") == session.incoming_headers.end())
    {
      throw HTTPEXCEPT("Error! No host provided! Routing failed!", 400, 400);
    }
  }
  return 0;
}

int HttpServer::prepareSession(HttpSession& session)
{
  session.outgoing_headers["content-type"] = "text/html";
  session.status_code = 200;
  return 0;
}

int HttpServer::checkResponse(HttpSession& session)
{
  if(session.outgoing_headers.find("date") != session.outgoing_headers.end()) session.outgoing_headers.erase("date");
  if(session.outgoing_headers.find("server") != session.outgoing_headers.end()) session.outgoing_headers["server"] = "YO Integrated Webserver";
  return 0;
}

//To safeguard FILE pointers. Just a quickie! :)
class FileLock
{
public:
  FILE* fil;
  FileLock(FILE* f) : fil(f)
  {
  }

  FileLock() : fil(NULL)
  {}

  ~FileLock()
  {
    if(fil != NULL)
    {
      fclose(fil);
      fil = NULL;
    }
  }
};

int HttpServer::sendResponse(HttpSession& session)
{
  FileLock lock;
  if(session.response.type == DataSource::File)
  {
    lock.fil = fopen(session.response.data.c_str(), "r");
    if(lock.fil == NULL)
      throw HTTPEXCEPT("Error. Unable to load specified file!", 404, 404);
  }

  if(session.status_string.size() == 0) session.status_string = getStatusString(session.status_code);
  //Send the first response line (terminated by \r\n)
  session.connection->write(getRequestProtocol(session.information) + " " + boost::to_string(session.status_code) + " " + session.status_string + "\r\n");

  session.connection->write("Date: " + timestamp() + "\r\n");

  for(boost::unordered_map<std::string, std::string>::const_iterator it = session.outgoing_headers.begin(); it != session.outgoing_headers.end(); ++it)
  {
    session.connection->write(it->first + ": " + it->second + "\r\n");
  }

  for(boost::unordered_map<std::string, std::string>::const_iterator it = session.outgoing_cookies.begin(); it != session.outgoing_cookies.end(); ++it)
  {
    session.connection->write("Set-Cookie: " + it->first + "=" + it->second + "\r\n");
  }

  if(session.response.type == DataSource::String)
  {
    session.connection->write("Content-Length: " + boost::to_string(session.response.data.size()) + "\r\n\r\n");
    session.connection->write(session.response.data + "\r\n");
  }
  else if(session.response.type == DataSource::File)
  {
    fseek(lock.fil, 0, SEEK_END);
    size_t pos = ftell(lock.fil);
    fseek(lock.fil, 0, SEEK_SET);
    session.connection->write_fd(fileno(lock.fil), pos);
    session.connection->write("\r\n");
  }

  return 0;
}

void HttpServer::requestHandler(HttpSession& session)
{
  session.response.type = DataSource::String;
  session.response.data = "Hello World!";
  session.outgoing_headers["content-type"] = "text/plain";
}
