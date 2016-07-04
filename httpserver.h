#ifndef HTTPSERVER_H
#define HTTPSERVER_H
#include "tcpserver.h"

#include <boost/asio.hpp>
#include <boost/unordered_map.hpp>

#include <string>

namespace http
{
  class HttpServer;
  class HttpSession;
  class DataSource;

  typedef boost::asio::ip::tcp::socket Socket;
  typedef uint32_t HttpOptions;
  typedef void (*HttpServerWorkerFunction)(HttpSession&);

  //16-bit bitfield containing Http options
  //Contains socket information as well as request information
  const static HttpOptions Http1_0 = 0x80000000;
  const static HttpOptions Http1_1 = 0x40000000;
  const static HttpOptions Http2_0 = 0x20000000;
  const static HttpOptions Spdy = 0x10000000;
  const static HttpOptions SslEnabled = 0x8000000;
  const static HttpOptions CompressionEnabled = 0x4000000;
  const static HttpOptions Websockets = 0x2000000;
  const static HttpOptions GetRequest = 0x1000000;
  const static HttpOptions PostRequest = 0x800000;
  const static HttpOptions PutRequest = 0x400000;
  const static HttpOptions DeleteRequest = 0x200000;
  const static HttpOptions ConnectRequest = 0x100000;
  const static HttpOptions OptionsRequest = 0x80000;
  const static HttpOptions TraceRequest = 0x40000;
  const static HttpOptions HttpKeepalive = 0x20000;
  const static HttpOptions PostUrlencoded = 0x8000;
  const static HttpOptions PostMultipart = 0x4000;

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

  class HttpSession
  {
    friend class HttpServer;
  private:
    boost::shared_ptr<Socket> socket;
    boost::asio::streambuf streambuf;
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


  class HttpServer
  {
  private:

    boost::asio::io_service ioService;
    short listeningPort;
    bool isServerRunning;

    std::string get_password() const;
    void acceptor();
    void runner(boost::shared_ptr<http::Socket> socket);

    std::string readLine(HttpSession& session, size_t len);

    //Prepare for handing HTTP session to the worker function
    void parseHttpRequest(boost::shared_ptr<Socket> sock);
    void processRequest(HttpSession& session);
      void parseGetQueries(HttpSession& session);
      void parseHeaders(HttpSession& session);
      void parsePostQueries(HttpSession& session);

    void checkRequest(HttpSession& session);
    void prepareSession(HttpSession& session);

    //Check and send the response
    void checkSessionResponse(HttpSession& session);
    void sendResponse(HttpSession& session);

  public:

    HttpServer();
    ~HttpServer();

    void start();

    short getPort();
    short setPort(short listeningPort);

    HttpServerWorkerFunction workerFunction;
  };


  std::string timestamp();
}

#endif // HTTPSERVER_H
