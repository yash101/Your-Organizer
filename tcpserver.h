#ifndef TCPSERVER_H
#define TCPSERVER_H
#include <boost/asio.hpp>
#include <boost/thread.hpp>

namespace sock
{
  class TcpServer;
  class TcpServerConnection;

  typedef boost::shared_ptr<boost::asio::ip::tcp::socket> SocketPointer;
  typedef void (*TcpServerFunction)(TcpServerConnection& connection);

  class TcpServerConnection
  {
  private:
  public:
    SocketPointer socket;
  };

  class TcpServer
  {
  private:
    boost::asio::io_service service;

    void acceptor();
    void runner(boost::shared_ptr<TcpServerConnection> connection);
  protected:
    short listPort;
    bool isRunning;
  public:
    TcpServer();
    TcpServerFunction worker;

    void start();

    short setPort(short listPort);
    short getPort();
  };
}
#endif // TCPSERVER_H
