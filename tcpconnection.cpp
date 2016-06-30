#include "tcpconnection.h"
#include "tcpsockets.h"
#include "exceptions.h"

#include <string.h>
#include <new>

#ifndef _WIN32

#include <errno.h>
#include <netinet/in.h>
#include <unistd.h>
#define SOCKET_INVALID(fd) (fd < 0)
#define SOCKET_NOERROR(x) (x > 0)
#define FD_INITIALIZER -1

#else

#include <Windows.h>
#define SOCKET_INVALID(fd) (fd == INVALID_SOCKET)
#define SOCKET_NOERROR(x) (x != SOCKET_ERROR)
#define FD_INITIALIZER INVALID_SOCKET

#endif

sock::Connection::Connection() :
  _fd(FD_INITIALIZER)
{
  _address = new(std::nothrow) struct sockaddr_in;
  _remoteAddress = new(std::nothrow) struct sockaddr_in;
  _timeout = new(std::nothrow) struct timeval;

  if(_address == NULL || _remoteAddress == NULL || _timeout == NULL)
    throw EXCEPTION("Unable to allocate memory!", (int) NULL);
}

sock::Connection::~Connection()
{
  if(_address != NULL) delete (struct sockaddr_in*) _address;
  if(_remoteAddress != NULL) delete (struct sockaddr_in*) _remoteAddress;
  if(_timeout != NULL) delete (struct timeval*) _timeout;

  if(!SOCKET_INVALID(_fd))
  {
#ifdef _WIN32
    shutdown(_fd, SHUT_BOTH);
    closesocket(_fd);
#else
    shutdown(_fd, SHUT_RDWR);
    close(_fd);
#endif
  }
}


sock::SOCK_RW_RET sock::Connection::read_int(void* buffer, size_t len, int flags)
{
  while(true)
  {
    sock::SOCK_RW_RET ret = recv(this->_fd, buffer, len, flags);
    if(ret <= (SOCK_RW_RET) len || ret < 0)
    {
      if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR || errno == ENOMEM)
        continue;
      else if(errno == EBADF || errno == ECONNREFUSED || errno == EFAULT || errno == EINVAL || errno == ENOTCONN || errno == ENOTSOCK)
        throw EXCEPTION("There is an issue with the socket or the connection. Thus, it is important that we end this connection!", errno);
    }

    return ret;
  }
}

sock::SOCK_RW_RET sock::Connection::write_int(void* buf, size_t len, int flags)
{
  char* buffer = (char*) buf;
  while(true)
  {
    ssize_t ret = send(this->_fd, buffer, len, flags);
    if(ret < (SOCK_RW_RET) len || ret < 0)
    {
      //We should continue to try to resend
      if(errno == EINTR  || errno == ENOBUFS || errno == ENOMEM)
      {
        continue;
      }

      if(errno == EACCES || errno == EBADF || errno == ECONNRESET || errno == EDESTADDRREQ || errno == EFAULT || errno == EINVAL || errno == EMSGSIZE || errno == ENOTCONN || errno == ENOTSOCK || errno == EOPNOTSUPP || errno == EPIPE)
      {
        throw EXCEPTION("There is an issue with the socket or the connection. Thus, it is important that we end this connection!", errno);
      }

      if(ret < (SOCK_RW_RET) len && ret >= 0)
        buffer += ret / sizeof(char);
    }
    else
    {
      return ret;
    }
  }
}

sock::SOCK_RW_RET sock::Connection::read(void* buffer, size_t len)
{
  return this->read_int(buffer, len, MSG_NOSIGNAL);
}

sock::SOCK_RW_RET sock::Connection::read(char& ch)
{
  return this->read_int(&ch, sizeof(char), MSG_NOSIGNAL);
}

sock::SOCK_RW_RET sock::Connection::write(void* buffer, size_t len)
{
  return this->write_int(buffer, len, MSG_NOSIGNAL);
}

sock::SOCK_RW_RET sock::Connection::write(char* str)
{
  return this->write_int(str, strlen(str) + sizeof(char), MSG_NOSIGNAL);
}


sock::SOCK_RW_RET sock::Connection::write(const char* str)
{
  return this->write_int((void*) str, strlen(str) + sizeof(char), MSG_NOSIGNAL);
}

sock::SOCK_RW_RET sock::Connection::write(char ch)
{
  return this->write_int((void*) &ch, sizeof(decltype(ch)), MSG_NOSIGNAL);
}
