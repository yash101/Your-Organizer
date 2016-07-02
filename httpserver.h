#ifndef HTTPSERVER_H
#define HTTPSERVER_H
#include "tcpserver.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <string>

namespace http
{
  class HttpServer;

  typedef boost::asio::ip::tcp::socket Socket;

  class HttpSocketSession
  {
  public:
    Socket& socket;
  };

  class HttpServer
  {
  private:

    boost::asio::io_service ioService;
    short listeningPort;
    bool isServerRunning;

    std::string get_password() const;
    void acceptor();
    void runner();
    void parseHttpRequest(boost::asio::ip::tcp::socket& sock);

  public:

    HttpServer();
    ~HttpServer();

    void start();

    short getPort();
    short setPort(short listeningPort);
  };
}

#endif // HTTPSERVER_H
