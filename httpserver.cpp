#include "httpserver.h"
#include "httpsupport.h"
#include "exceptions.h"
#include "config.h"

#include <boost/algorithm/string.hpp>

#include <sstream>

#define me (*this)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Configuration variables */
namespace vars
{
  static size_t maxlen_firstline;	//Maximum number of bytes downloaded in the first line ({METHOD} {URI} HTTP/{v}\r\n)
  static size_t max_hdrct;				//Maximum number of HTTP headers to download
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
    max_hdrct = conf::getConfigInt("http request headers maximum_count");
    max_hdrklen = conf::getConfigInt("http request headers maximum_key_length");
    max_hdrvlen = conf::getConfigInt("http request headers maximum_value_length");
    max_postlen = conf::getConfigInt("http request queries post maximum_count");
    max_postdata = conf::getConfigInt("http request queries pos maximum_data_usage");
    max_postkeylen = conf::getConfigInt("http request queries post maximum_key_length");
    max_ncookies = conf::getConfigInt("http request cookies maximum_count");
    max_cookiekeylen = conf::getConfigInt("http request cookies maximum_key_length");
    max_cookievallen = conf::getConfigInt("http request cookies maximum_value_length");
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Server code */

//Starts the server. This function listens for incoming connections
void http::HttpServer::start()
{
  if(!vars::isInit) vars::init();
  boost::asio::ip::tcp::acceptor acceptor(me.ioService, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v6(), me.listeningPort));
  while(true)
  {
    boost::shared_ptr<http::Socket> connection(new http::Socket(me.ioService));
    acceptor.accept(*connection);
    boost::thread thread(boost::bind(&http::HttpServer::runner, this, connection));
  }
}

//This function runs the web server function
//Makes sure no exceptions get past this. A single exception would cause
//a full server crash!
void http::HttpServer::runner(boost::shared_ptr<http::Socket> connection)
{
  try
  {
    me.parseHttpRequest(connection);
  }
  catch(std::exception& e)
  {}
}

//The HTTP server. This calls for the request to be parsed
void http::HttpServer::parseHttpRequest(boost::shared_ptr<http::Socket> socket)
{
  //Create the session object
  http::HttpSession session;
  session.socket = socket;

  //Process the request
  me.processRequest(session);
}

//Processes the request. Runs the functions, one after the other
void http::HttpServer::processRequest(http::HttpSession& session)
{
  //Read the first line
  std::string buf0 = me.readLine(session, vars::maxlen_firstline);
  std::vector<std::string> parts;
  boost::split(parts, buf0, boost::is_any_of(" "));
  if(parts.size() != 3)
  {
    throw EXCEPTION("Error! Unable to process request. Initial request line invalid!", 400);
  }

  //Fill out the bitset fields!
  session.information |= http::getRequestType(parts[0]);
  session.information |= http::getRequestProtocol(parts[2]);

  //Set the path!
  session.path_unprocessed = parts[1];


  //Process GET queries
  me.parseGetQueries(session);
  //Process HTTP headers
  me.parseHeaders(session);
  //Process POST queries
  me.parsePostQueries(session);
}

//Retrieves GET queries from the URL
void http::HttpServer::parseGetQueries(HttpSession& session)
{
  //Find the first question mark
  size_t qloc = session.path_unprocessed.find('?');
  //If there are no GET queries
  if(qloc == std::string::npos)
  {
    session.path = session.path_unprocessed;
  }
  //Again, no GET queries, but trailing question mark to remove
  else if(qloc == session.path_unprocessed.size() - 1)
  {
    session.path = session.path_unprocessed.substr(0, session.path_unprocessed.size() - 1);
  }
  //Actually parse GET queries
  else
  {
    //Retrieve the queries' string and break
    std::string queries = session.path_unprocessed.substr(qloc + 1, session.path_unprocessed.size());
    std::vector<std::string> pairs;
    boost::split(pairs, queries, boost::is_any_of("&"));

    //Process each query
    for(size_t i = 0; i < pairs.size(); i++)
    {
      size_t eloc = pairs[i].find('=');
      //If the equal sign does not exist or is the last character
      if(eloc == std::string::npos || pairs[i].back() == '=')
      {
        std::string key = pairs[i];
        if(key.back() == '=') key.pop_back();
        if(key.size() == 0) continue;
        session.get_queries[http::decodeURI(key)].data = "";
        session.get_queries[http::decodeURI(key)].type = http::DataSource::String;
      }
      //If it is a plain ol' GET request
      else
      {
        //If the field name is empty (Naughty Naughty!)
        if(eloc - 1 == 0) continue;

        std::string key = http::decodeURI(pairs[i].substr(0, eloc - 1));
        std::string value = http::decodeURI(pairs[i].substr(eloc + 1));
        session.get_queries[key].data = "";
        session.get_queries[key].type = http::DataSource::String;
      }
    }
  }
}

//Retrieves and parses all headers
void http::HttpServer::parseHeaders(http::HttpSession& session)
{
  //Number of cookies downloaded from client in session
  size_t dcookies = 0;
  //Download headers -- up to out limit
  for(size_t i = 0; vars::max_hdrct != 0 || i < vars::max_hdrct; i++)
  {
    //Download header
    std::string hdr = me.readLine(session, vars::max_hdrklen + vars::max_hdrvlen + 2);
    //Empty means no more headers
    if(hdr.size() == 0) break;
    //Find the [first] colon
    size_t pos = hdr.find(':');
    if(pos == std::string::npos)
    {
      //Parse a cookie -- actually, the cookie does not exist here!
      std::string chcookie = hdr;
      boost::to_lower(chcookie);
      boost::trim(chcookie);
      if(chcookie == "cookie")
      {
        i--;
        continue;
      }
      session.incoming_headers[chcookie] = "";
    }
    else
    {
      //Prepare header
      std::string key = hdr.substr(0, pos - 1);
      std::string value = hdr.substr(pos + 1, hdr.size());
      boost::to_lower(key);
      boost::trim(key);
      boost::trim(value);

      //Check if cookie header
      if(key == "cookie")
      {
        i--;
        dcookes++;
        if(dcookies > vars::max_ncookies);
        //Parse the cookie! (Nom num nom)
        size_t cpos = value.find('=');
        if(cpos == std::string::npos)
        {
          boost::to_lower(value);
          if(vars::max_cookiekeylen > 0 && value.size() > vars::max_cookiekeylen)
            throw EXCEPTION("Maximum supported bytes for HTTP request reached!", 413);
          session.incoming_cookies[value] = "";
        }
        else
        {
          std::string key = value.substr(0, cpos - 1);
          boost::trim(key);
          boost::to_lower(key);
          std::string val = value.substr(cpos + 1, value.size());
          boost::trim(val);
          if(vars::max_cookiekeylen > 0 && key.size() > vars::max_cookiekeylen)
            throw EXCEPTION("Maximum supported bytes for HTTP cookie request reached!", 413);
          if(vars::max_cookiekeylen > 0 && val.size() > vars::max_cookievallen)
            throw EXCEPTION("Maximum supported bytes for HTTP cookie request reached!", 413);
          session.incoming_cookies[key] = val;
        }
        continue;
      }

      session.incoming_headers[key] = value;
    }
  }
}

//Downloads and parses the POST queries
void http::HttpServer::parsePostQueries(http::HttpSession& session)
{
  std::string ctp = session.incoming_headers["content-type"];
  boost::to_lower(ctp);

  //If multipart request
  if(ctp.find("multipart/form-data"))
  {
  }
  else	//General POST request
  {
    size_t length = std::atoll(session.incoming_headers["content-length"]);
    if(length > vars::max_postlen)
      throw EXCEPTION("Cannot handle this much POST data! Asset too large for processing!", 413);
    std::vector<std::string> queries;

    char* data = new char[length + 2];
    memset((void*) data, 0, length + 2);

    boost::asio::read(session.socket, boost::asio::buffer(data, length));

    std::string str(data);
    delete data;

    boost::split(queries, str, boost::is_any_of("&"));

    for(std::vector<std::string>::const_iterator it = queries.begin(); it != queries.end(); ++it)
    {
      std::vector<std::string> parts;
      boost::split(parts, *it, boost::is_any_of("="));

      //If they sent ...&=&...
      if(parts.size() == 0) continue;
      //If they sent ...&x=&...
      if(parts.size() == 1)
      {
        http::decodeURI(parts[0]);
        session.post_queries[parts[0]].data = "";
        session.post_queries[parts[0]].type = http::DataSource::String;
      }
      //If they sent ...&x=y&...
      else
      {
        http::decodeURI(parts[0]);
        session.post_queries[parts[0]].data = parts[1];
        session.post_queries[parts[0]].type = http::DataSource::String;
      }
    }
  }
}


//Checks for the validity of the request
void http::HttpServer::checkRequest(http::HttpSession& session)
{
  if(session.information & http::Http1_1)
  {
    //We must have recieved the Host header. If not, we must send back 400 Bad Request!
    if(session.incoming_headers["host"].empty())
      throw EXCEPTION("Error. No \"host\" header specified. Cannot route request, as per HTTP 1.1+ standards!", 400);
  }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Functions "in the wild" */

http::HttpOptions http::getRequestType(std::string reqstr)
{
  if(reqstr == "GET") return http::GetRequest;
  else if(reqstr == "POST") return http::PostRequest;
  else if(reqstr == "PUT") return http::PutRequest;
  else if(reqstr == "DELETE") return http::DeleteRequest;
  else if(reqstr == "CONNECT") return http::ConnectRequest;
  else if(reqstr == "TRACE") return http::TraceRequest;
  else throw EXCEPTION(std::string("Error: Request action requested (" + reqstr + ") not supported!").c_str(), 405);
}

http::HttpOptions http::getRequestProtocol(std::string reqstr)
{
  if(reqstr == "HTTP/1.0") return http::Http1_0;
  else if(reqstr == "HTTP/1.1") return http::Http1_1;
  else if(reqstr == "HTTP/2.0") return http::Http2_0;
  else throw EXCEPTION(std::string("Error: Protocol requested (" + reqstr + ") not supported!").c_str(), 505);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Class basic functions/setup functions */

http::HttpServer::HttpServer() :
  listeningPort(0),
  isServerRunning(false)
{}

http::HttpServer::~HttpServer()
{}

//Reads a line from the socket
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
      throw EXCEPTION("Maximum supported bytes were read!", 413);
    och = ch;
  }
  std::string string = str.str();
  string.pop_back();
  return string;
}
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
