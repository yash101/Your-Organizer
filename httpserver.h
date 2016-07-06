#ifndef HTTPSERVER_H
#define HTTPSERVER_H
#include "tcpserver.h"

#include <boost/unordered_map.hpp>

#include <string>

namespace http
{
  class HttpServer;
  class HttpSession;
  class DataSource;

  typedef uint32_t HttpOptions;
  typedef void (*HttpServerWorkerFunction)(HttpSession&);

#define SET_BIT(n)(1 << n)
  //16-bit bitfield containing Http options
  //Contains socket information as well as request information
  const static HttpOptions Http1_0 = SET_BIT(1);
  const static HttpOptions Http1_1 = SET_BIT(2);
  const static HttpOptions Http2_0 = SET_BIT(3);
  const static HttpOptions GetRequest = SET_BIT(4);
  const static HttpOptions PostRequest = SET_BIT(5);
  const static HttpOptions PutRequest = SET_BIT(6);
  const static HttpOptions DeleteRequest = SET_BIT(7);
  const static HttpOptions ConnectRequest = SET_BIT(8);
  const static HttpOptions OptionsRequest = SET_BIT(9);
  const static HttpOptions TraceRequest = SET_BIT(10);
  const static HttpOptions Websockets = SET_BIT(11);
  const static HttpOptions HttpKeepalive = SET_BIT(12);
  const static HttpOptions PostUrlencoded = SET_BIT(13);
  const static HttpOptions PostMultipart = SET_BIT(14);
#undef SET_BIT

  class DataSource
  {
  public:
    std::string data;
    char type;

    const static char String = 0x0;
    const static char File = 0x2;
    const static char NoData = 0x4;

    DataSource() :
      type(NoData)
    {}
  };

  HttpOptions getRequestType(std::string reqstr);
  HttpOptions getRequestProtocol(std::string reqstr);
  std::string getRequestProtocol(HttpOptions options);
  std::string getStatusString(int code);
  std::string timestamp();

  class HttpSession
  {
    friend class HttpServer;
  private:
    srv::TcpServerConnection* connection;
    HttpOptions information;

    std::string path;
    std::string path_unprocessed;

    boost::unordered_map<std::string, DataSource> get_queries;
    boost::unordered_map<std::string, DataSource> post_queries;
    boost::unordered_map<std::string, std::string> incoming_headers;
    boost::unordered_map<std::string, std::string> incoming_cookies;

  public:
    boost::unordered_map<std::string, std::string> outgoing_headers;
    boost::unordered_map<std::string, std::string> outgoing_cookies;

    short int status_code;
    std::string status_string;
    DataSource response;

    HttpOptions getRequestInformation();
    const std::string getPath();
    const std::string getUnprocessedPath();
    const DataSource getQuery(std::string key);
    const DataSource getQueryGet(std::string key);
    const DataSource getQueryPost(std::string key);
  };

  class WebsocketsSession
  {
    friend class HttpServer;

  private:

    srv::TcpServerConnection* connection;
    HttpSession* httpSession;
    std::string websocketKey;
    int websocketVersion;

  public:
  };


  class HttpServer : public srv::TcpServer
  {
  private:

    virtual void worker(srv::TcpServerConnection& connection);
    int processRequestLine(HttpSession& session);
      void processRequestUrl(HttpSession& session);
    int processHeaders(HttpSession& session);
    int processPostQueries(HttpSession& session);

    //WebSocket stuff
    int initializeWebsockets(HttpSession& session, WebsocketsSession& wsession);
    int websocketHandshake(HttpSession& session, WebsocketsSession& wsession);

    //HTTP Stuff
    int checkRequest(HttpSession& session);
    int prepareSession(HttpSession& session);
    int checkResponse(HttpSession& session);
    int sendResponse(HttpSession& session);

  protected:

    virtual void requestHandler(HttpSession& session);
    virtual void websocketInit(WebsocketsSession& session);
    virtual void websocketWorker(WebsocketSession& session);

  public:

    HttpServer();
  };
}

#endif // HTTPSERVER_H
