#include "http_internals.h"
#include "httpserver.h"
#include "../base/exceptions.h"

#include <sstream>
#include <time.h>

#include <boost/algorithm/string.hpp>

#define me (*this)

//Converts a character to hex code
static inline unsigned char charToHex(unsigned char in)
{
  return (in + ((in > 9) ? ('A' - 10) : '0'));
}

//Converts hex code to a char
static inline unsigned char hexToChar(unsigned char in)
{
  if(in <= '9' && in >= '0')
    in -= '0';
  else if(in <= 'f' && in >= 'a')
    in -= 'a' - 10;
  else if(in <= 'F' && in >= 'A')
    in -= 'A' - 10;
  else
    in = 0;
  return in;
}

//URL encodes an std::string
std::string http::encodeURI(std::string in)
{
  std::stringstream str;
  for(std::string::const_iterator it = in.begin(); it != in.end(); ++it)
  {
    if((*it >= 'a' && *it <= 'z') || (*it <= 'A' && *it <= 'Z') || (*it >= '0' && *it <= '9'))
    {
      str << *it;
    }
    else if(*it == ' ')
    {
      str << '+';
    }
    else
    {
      str << '%' << charToHex(*it >> 4) << charToHex(*it % 16);
    }
  }
  return str.str();
}

//URL decodes an std::string
std::string http::decodeURI(std::string in)
{
  std::stringstream str;

  for(size_t i = 0; i < in.size(); i++)
  {
    if(in[i] == '+')
    {
      str << ' ';
    }
    else if(in[i] == '%' && in.size() > i + 2)
    {
      str << (hexToChar(in[i + 1] << 4) | hexToChar(in[i + 2]));
      i += 2;
    }
    else
    {
      str << in[i];
    }
  }
  return str.str();
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* HttpSession functions */

//Retrieves the path
const std::string http::HttpSession::getPath()
{
  return me.path;
}

//Retrieves the path, before processing
const std::string http::HttpSession::getUnprocessedPath()
{
  return me.path_unprocessed;
}

//Retrieves a query
const http::DataSource http::HttpSession::getQuery(std::string key)
{
  if(me.get_queries.count(key) > 0)
    return me.get_queries[key];
  else
    return me.post_queries[key];
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Functions "in the wild" */

//Parses the request type
http::HttpOptions http::getRequestType(std::string reqstr)
{
  if(reqstr == "GET") return http::GetRequest;
  else if(reqstr == "POST") return http::PostRequest;
  else if(reqstr == "PUT") return http::PutRequest;
  else if(reqstr == "DELETE") return http::DeleteRequest;
  else if(reqstr == "CONNECT") return http::ConnectRequest;
  else if(reqstr == "TRACE") return http::TraceRequest;
  else throw HTTPEXCEPT(std::string("Error: Request action requested (" + reqstr + ") not supported!").c_str(), 405, 405);
}


//Parses the protocol version
http::HttpOptions http::getRequestProtocol(std::string reqstr)
{
  if(reqstr == "HTTP/1.0") return http::Http1_0;
  else if(reqstr == "HTTP/1.1") return http::Http1_1;
  else if(reqstr == "HTTP/2.0") return http::Http2_0;
  else throw HTTPEXCEPT(std::string("Error: Protocol requested (" + reqstr + ") not supported!").c_str(), 505, 505);
}
std::string http::getRequestProtocol(HttpOptions type)
{
  if(type & Http1_0) return "HTTP/1.0";
  else if(type & Http1_1) return "HTTP/1.1";
  else if(type & Http2_0) return "HTTP/1.0";
  else return "UNKNOWN";
}

//Used for generating HTTP status codes
static boost::unordered_map<int, std::string> httpStatusCodes;
static struct HSC_INIT
{
  HSC_INIT()
  {
#define c(a, b) httpStatusCodes[a] = b;
    c(100, "Continue");
    c(101, "Switching Protocols");
    c(200, "OK");
    c(201, "Created");
    c(202, "Accepted");
    c(203, "Non-Authorative Information");
    c(204, "No Content");
    c(205, "Reset Content");
    c(206, "Partial Content");
    c(300, "Multiple Choices");
    c(301, "Moved Permanently");
    c(302, "Found");
    c(303, "See Other");
    c(304, "Not Modified");
    c(305, "Use Proxy");
    c(306, "STATUS_CODE_UNUSED");
    c(307, "Temporary Redirect");
    c(400, "Bad Request");
    c(401, "Unauthorized");
    c(402, "Payment Required");
    c(403, "Forbidden");
    c(404, "Not Found");
    c(405, "Method not Allowed");
    c(406, "Not Acceptable");
    c(407, "Proxy Authentication Required");
    c(408, "Request Timeout");
    c(409, "Conflict");
    c(410, "Gone");
    c(411, "Length Required");
    c(412, "Precondition Failed");
    c(413, "Request Entity too Large");
    c(414, "Request-URI too Long");
    c(415, "Unsupported Media Type");
    c(416, "Requested Range not Satisfiable");
    c(417, "Expectation Failed");
    c(500, "Internal Server Error");
    c(501, "Not Implemented");
    c(502, "Bad Gateway");
    c(503, "Service Unavailable");
    c(504, "Gateway Timeout");
    c(505, "HTTP Version not Supported");
#undef c
  }
} hsc_init;

std::string http::getStatusString(int code)
{
  if(httpStatusCodes.find(code) == httpStatusCodes.end()) return "Unknown Status Code";
  else return httpStatusCodes[code];
}

//Used for generating timestamp for "Date" header field
static const char* const days[] =
{ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static const char* const months[] =
{ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
std::string http::timestamp()
{
  time_t localtime;
  struct tm* tme;
  localtime = time(NULL);
  tme = gmtime(&localtime);

  std::stringstream str;
  str << days[tme->tm_wday]
      << ", "
      << tme->tm_mday << " "
      << months[tme->tm_mon] << " "
      << tme->tm_year + 1900 << " "
      << tme->tm_hour << ":"
      << tme->tm_min << ":"
      << tme->tm_sec << " GMT";

  return str.str();
}
