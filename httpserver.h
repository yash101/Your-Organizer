#ifndef HTTPSERVER_H
#define HTTPSERVER_H
#include "tcpserver.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <string>

namespace http
{
  class HttpServer;

  typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> SslSocket;
  typedef boost::asio::ip::tcp::socket Socket;

  class HttpSocketSession
  {
  public:
    boost::asio::ip::socket Socket;
  };

  class HttpServer
  {
  private:

    boost::asio::io_service ioService;
    bool isServerRunning;
    bool sslEnable;
    short listeningPort;
    bool safeMode; 				//Prevents retrieval of certificate data from class
    std::string password;
    std::string certificateChainFile;
    std::string privateKeyFile;
    std::string temporaryDhFile;

    std::string get_password() const;
    void acceptor();
    void runner();
    void parseHttpRequest(boost::asio::ip::tcp::socket& sock, boost::asio::ssl::context& context);

  public:

    HttpServer();
    ~HttpServer();

    void start();

    short getPort();
    short setPort(short listeningPort);
    bool enableSSL();
    bool disableSSL();
    bool isSSLEnabled();
    std::string setPassword(std::string pass);
    std::string getPassword();
    std::string setCertificateChainFile(std::string path);
    std::string getCertificateChainFile();
    std::string setPrivateKeyFile(std::string path);
    std::string getPrivateKeyFile();
    std::string setTemporaryDhFile(std::string path);
    std::string getTemporaryDhFile();

    void enableSafeMode();
    bool isSafeModeEnabled();
  };
}

#endif // HTTPSERVER_H
