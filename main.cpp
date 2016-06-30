#include "tcpsockets.h"
#include <stdio.h>
#include <thread>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

const int max_length = 1024;

void session(boost::shared_ptr<boost::asio::ip::tcp::socket> ptr)
{
  try
  {
    while(true)
    {
      char data[max_length];
      boost::system::error_code error;
      size_t length = ptr->read_some(boost::asio::buffer(data), error);
      if(error == boost::asio::error::eof)
        break;
      else if(error)
        throw boost::system::system_error(error);

      boost::asio::write(*ptr, boost::asio::buffer(data, length));
    }
  }
  catch(std::exception& e)
  {
    fprintf(stderr, "Error: Exception in thread: [%s]\n", e.what());
  }
}

void server(boost::asio::io_service& service, short port)
{
  boost::asio::ip::tcp::acceptor acceptor(service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
  while(true)
  {
    boost::shared_ptr<boost::asio::ip::tcp::socket> sock(new boost::asio::ip::tcp::socket(service));
    acceptor.accept(*sock);
    boost::thread t(boost::bind(session, sock));
  }
}

int main(int argc, char** argv)
{
  try
  {
    boost::asio::io_service service;
    server(service, 8080);
  }
  catch(std::exception& e)
  {
    fprintf(stderr, "Error: Exception: [%s]\n", e.what());
  }

  return 0;
}
