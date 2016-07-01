#include "tcpserver.h"
#include "exceptions.h"


sock::TcpServer::TcpServer() :
  listPort(0),
  isRunning(false)
{}

void sock::TcpServer::acceptor()
{
  boost::asio::ip::tcp::acceptor server(service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v6(), listPort));
  while(true)
  {
    boost::shared_ptr<TcpServerConnection> connection(new TcpServerConnection);
    connection->socket = SocketPointer(new boost::asio::ip::tcp::socket(service));
    server.accept(*connection->socket);
    boost::thread thread(boost::bind(&sock::TcpServer::runner, this, connection));
    thread.detach();
  }
}

void sock::TcpServer::runner(boost::shared_ptr<TcpServerConnection> connection)
{
  try
  {
    this->worker(*connection);
  }
  catch(std::exception& e)
  {
    fprintf(stderr, "Exception thrown from worker function! what(): [%s]\n", e.what());
  }
}

void sock::TcpServer::start()
{
  if(this->isRunning)
    throw EXCEPTION("Server already running!", -1);
  this->isRunning = true;
  this->acceptor();
}

short sock::TcpServer::setPort(short port)
{
  if(this->isRunning)
    throw EXCEPTION("Server already running!", -1);
  return (this->listPort = port);
}

short sock::TcpServer::getPort()
{
  return listPort;
}
