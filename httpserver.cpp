#include "httpserver.h"
#include "exceptions.h"
#define me (*this)

void http::HttpServer::start()
{
  if(me.sslEnable)
  {
    boost::asio::ssl::context sslContext(me.ioService, boost::asio::ssl::context::sslv23);
    boost::asio::ip::tcp::acceptor acceptor(me.ioService, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v6(), me.listeningPort));

    sslContext.set_options(
        boost::asio::ssl::context::default_workarounds |
        boost::asio::ssl::context::no_sslv2 |
        boost::asio::ssl::context::single_dh_use
    );

    sslContext.set_password_callback(boost::bind(&http::HttpServer::get_password, this));
    sslContext.use_certificate_chain_file(me.certificateChainFile);
    sslContext.use_private_key_file(me.privateKeyFile, boost::asio::ssl::context::pem);
    sslContext.use_tmp_dh_file(me.temporaryDhFile);

    while(true)
    {
      sock::SocketPointer socket(new boost::asio::ip::tcp::socket(me.ioService));
      acceptor.accept(*socket);
      boost::thread thread(boost::bind(&http::HttpServer::runner, this, socket, sslContext));
      thread.detach();
    }
  }
}

void http::HttpServer::runner()
{
  try
  {
  }
  catch(std::exception& e)
  {}
}

void http::HttpServer::parseHttpRequest(boost::asio::ip::tcp::socket& socket, boost::asio::ssl::context& context)
{
  if(me.sslEnable)
  {
    http::SslSocket socket(me.ioService, context);
    socket.lowest_layer() = socket;
  }
}


/* Class basic functions/setup functions */

http::HttpServer::HttpServer() :
  listeningPort(0),
  isServerRunning(false),
  sslEnable(false)
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

bool http::HttpServer::enableSSL()
{
  ERRR
  bool ret = me.sslEnable;
  sslEnable = true;
  return ret;
}

bool http::HttpServer::disableSSL()
{
  ERRR
  bool ret = me.sslEnable;
  sslEnable = false;
  return ret;
}

bool http::HttpServer::isSSLEnabled()
{
  return sslEnable;
}

std::string http::HttpServer::setPassword(std::string pass)
{
  ERRR;
  SET(me.password, pass);
}

std::string http::HttpServer::getPassword()
{
  GET(me.password);
}

std::string http::HttpServer::setCertificateChainFile(std::string path)
{
  ERRR;
  SET(me.certificateChainFile, path);
}

std::string http::HttpServer::getCertificateChainFile()
{
  GET(me.certificateChainFile);
}

std::string http::HttpServer::setPrivateKeyFile(std::string path)
{
  ERRR;
  SET(me.privateKeyFile, path);
}

std::string http::HttpServer::getPrivateKeyFile()
{
  GET(me.privateKeyFile);
}

std::string http::HttpServer::setTemporaryDhFile(std::string path)
{
  ERRR;
  SET(me.temporaryDhFile, path);
}

std::string http::HttpServer::getTemporaryDhFile()
{
  GET(me.temporaryDhFile);
}

void http::HttpServer::enableSafeMode()
{
  safeMode = true;
}

bool http::HttpServer::isSafeModeEnabled()
{
  return safeMode;
}

std::string http::HttpServer::get_password() const
{
  return me.password;
}
