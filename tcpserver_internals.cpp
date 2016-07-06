#include "tcpserver.h"
#include "exceptions.h"
#include "automaticmutex.h"

#include <new>
#include <string.h>
#include <string>
#include <sstream>

/*  Preprocessor stuff to make things easier/more readable!  */

#ifdef _WIN32

#pragma comment(lib, "Ws2_32.lib")
#include <Windows.h>

static WSADATA WsaData;
static WORD WsaVersionRequested = NULL;
static int WsaError = 0;
static bool WsaInitialized = false;

#else

#include <netinet/in.h>
#include <sys/sendfile.h>
#include <unistd.h>

#endif

#define me (*this)
#define MUTEX base::AutoMutex<boost::mutex>
static bool sockets_initialized = false;
using namespace srv;




//////////////////////////////////////////////////////////////////////////////////////////////////
/* Socket initialization/deinitialization functions. Mainly for Winsock[2]! */
#ifdef _WIN32
void srv::initializeSockets()
{
  if(sockets_initialized)
    return WsaError;
  if(!WsaInitialized)
  {
    WsaVersionRequested = MAKEWORD(2, 2);
    WsaError = WSAStartup(WsaVersionRequested, WsaData);

    if(WsaError == 0)
      WsaInitialized = true;
  }

  return WsaError;
}

void uninitializeSockets()
{
  if(WsaInitialized)
    return WSACleanup();
}
#else
void srv::initializeSockets()
{
  if(sockets_initialized) return;
  sockets_initialized = true;
}

void uninitializeSockets()
{
  return;
}
#endif




//////////////////////////////////////////////////////////////////////////////////////////////////
/* TCP Server class stuff */
TcpServer& TcpServer::setPort(int port)
{
  if(me._isServerRunning)
    throw EXCEPTION("ERROR. Cannot change the port of an already-running TCP server!", -1);
  me._listeningPort = port;
  return me;
}

int TcpServer::getPort()
{
  return me._listeningPort;
}

TcpServer& TcpServer::setMaximumConcurrentClients(size_t num)
{
  MUTEX mtx(me._connectedClientsMutex);
  me._maxSimultaneousClients = num;
  mtx.unlock();
  return me;
}

size_t TcpServer::getMaximumConcurrentClients()
{
  return me._maxSimultaneousClients;
}

TcpServer& TcpServer::setTcpQueueSize(int qs)
{
  if(me._isServerRunning)
    throw EXCEPTION("ERROR. Cannot change TCP queue size while the server is running!", -1);
  me._tcpConnectionQueue = qs;
  return me;
}

int TcpServer::getTcpQueueSize()
{
  return me._tcpConnectionQueue;
}

TcpServer& TcpServer::setSocketTimeout(long usec, long sec)
{
  MUTEX mtx(me._timeoutMutex);
  ((struct timeval*) me._timeoutStructure)->tv_sec = sec;
  ((struct timeval*) me._timeoutStructure)->tv_usec = usec;
  mtx.unlock();
  return me;
}

TcpServer& TcpServer::setSocketTimeoutMicroseconds(long usec)
{
  MUTEX mtx(me._timeoutMutex);
  ((struct timeval*) me._timeoutStructure)->tv_usec = usec;
  mtx.unlock();
  return me;
}

TcpServer& TcpServer::setSocketTimeoutSeconds(long sec)
{
  MUTEX mtx(me._timeoutMutex);
  ((struct timeval*) me._timeoutStructure)->tv_sec = sec;
  mtx.unlock();
  return me;
}

long TcpServer::getSocketTimeoutMicroseconds()
{
  return ((struct timeval*) me._timeoutStructure)->tv_usec;
}

long TcpServer::getSocketTimeoutSeconds()
{
  return ((struct timeval*) me._timeoutStructure)->tv_sec;
}

bool TcpServer::isServerRunning()
{
  return me._isServerRunning;
}

void TcpServer::initializeAllVariables()
{
  me._addressStructure = NULL;
  me._timeoutStructure = NULL;

  me._fileDescriptor = SOCKET_INITIALIZER;
  me._listeningPort = 0;
  me._isServerRunning = false;
  me._simultaneousClients = 0;
  me._maxSimultaneousClients = 0;
  me._tcpConnectionQueue = 5;

  me._addressStructure = (void*) new (std::nothrow) struct sockaddr_in6;
  me._timeoutStructure = (void*) new (std::nothrow) struct timeval;
  if(me._timeoutStructure != NULL)
  {
    ((struct timeval*) me._timeoutStructure)->tv_sec = 0;
    ((struct timeval*) me._timeoutStructure)->tv_usec = 0;
  }
  if(me._addressStructure != NULL)
  {
    memset(me._addressStructure, 0, sizeof(struct sockaddr_in6));
  }
  if(me._addressStructure == NULL || me._timeoutStructure == NULL)
  {
    if(me._addressStructure != NULL) delete (struct sockaddr_in6*) me._addressStructure;
    if(me._timeoutStructure != NULL) delete (struct timeval*) me._timeoutStructure;

    throw EXCEPTION("Unable to allocate memory for server!", -1);
  }
}

void TcpServer::destroyAllVariables()
{
  if(me._addressStructure != NULL) delete (struct sockaddr_in6*) me._addressStructure;
  if(me._timeoutStructure != NULL) delete (struct timeval*) me._timeoutStructure;
}

TcpServer::TcpServer()
{
  me.initializeAllVariables();
}

TcpServer::~TcpServer()
{
  me.destroyAllVariables();
}




//////////////////////////////////////////////////////////////////////////////////////////////////
/* TCP Server Connection Functions */

TcpServerConnection::TcpServerConnection() :
  _fileDescriptor(SOCKET_INITIALIZER),
  _listeningServer(NULL),
  _isClientCounterActive(false)
{
  me._addressStructure = (void*) new (std::nothrow) struct sockaddr_in6;
  me._timeoutStructure = (void*) new (std::nothrow) struct timeval;
}

TcpServerConnection::~TcpServerConnection()
{
  if(me._fileDescriptor != SOCKET_INITIALIZER)
  {
#ifndef _WIN32
    shutdown(me._fileDescriptor, SHUT_RDWR);
    close(me._fileDescriptor);
#else
    shutdown(me._fileDescriptor, SHUT_BOTH);
    closesocket(me._fileDescriptor);
#endif
  }

  if(me._addressStructure != NULL)
    delete (struct sockaddr_in6*) me._addressStructure;
  if(me._timeoutStructure != NULL)
    delete (struct timeval*) me._timeoutStructure;

  if(me._isClientCounterActive)
  {
    if(me._listeningServer != NULL)
    {
      base::AutoMutex<boost::mutex> mtx;
      me._listeningServer->_simultaneousClients--;
      mtx.unlock();
    }
  }
}

SOCK_IO_RET TcpServerConnection::read_internal(void* buffer, size_t len, int flags)
{
  SOCK_IO_RET ret = recv(me._fileDescriptor, buffer, len, flags);
  while(true)
  {
    if(ret <= 0)
    {
      if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR || errno == ENOMEM)
        continue;
      else if(errno == EBADF || errno == ECONNREFUSED || errno == EFAULT || errno == EINVAL || errno == ENOTCONN || errno == ENOTSOCK)
        throw EXCEPTION("There is an issue with the socket or the connection. Thus, it is important that we end this connection!", error::SocketReceiveFailed);
    }

    return ret;
  }
}

SOCK_IO_RET TcpServerConnection::write_internal(void* buffer, size_t len, int flags)
{
  SOCK_IO_RET ret;
  char* buf = (char*) buffer;
  while(len > 0)
  {
    ret = send(me._fileDescriptor, buf, len, flags);
    if(ret <= 0)
    {
      if(errno == EINTR) continue;
      else
        throw EXCEPTION("Error writing data to socket!", errno);
    }
    buf += ret;
    len -= ret;
  }
  return ret;
}

SOCK_IO_RET TcpServerConnection::read(char& ch)
{
  return me.read_internal(&ch, sizeof(char), MSG_NOSIGNAL);
}

SOCK_IO_RET TcpServerConnection::read(void* buffer, size_t len)
{
  return me.read_internal(buffer, len, MSG_NOSIGNAL);
}

SOCK_IO_RET TcpServerConnection::read(std::string& string, size_t mxlen)
{
  char* str = new char[mxlen + 1];
  str[mxlen] = '\0';
  SOCK_IO_RET r = 0;
  try
  {
    r = me.read_internal(str, mxlen, MSG_NOSIGNAL);
  }
  catch(Exception& e)
  {
    delete[] str;
    throw e;
  }

  string = std::string(str);
  delete[] str;
  return r;
}

SOCK_IO_RET TcpServerConnection::write(char ch)
{
  return me.write_internal(&ch, sizeof(char), MSG_NOSIGNAL);
}

SOCK_IO_RET TcpServerConnection::write(void* buffer, size_t len)
{
  return me.write_internal(buffer, len, MSG_NOSIGNAL);
}

SOCK_IO_RET TcpServerConnection::write(char* str)
{
  return me.write_internal(str, strlen(str), MSG_NOSIGNAL);
}

SOCK_IO_RET TcpServerConnection::write(const char* str)
{
  return me.write_internal((void*) str, strlen(str), MSG_NOSIGNAL);
}

SOCK_IO_RET TcpServerConnection::write(std::string str)
{
  return me.write_internal((void*) str.c_str(), str.size(), MSG_NOSIGNAL);
}

SOCK_IO_RET TcpServerConnection::write_fd(SOCK_HANDLE handle, size_t dataRead)
{
  size_t pos = 0;
  char* buffer = new char[8192];
  SOCK_IO_RET ret = 0;
  while(pos > 0)
  {

#ifndef _WIN32

    ret = sendfile(me._fileDescriptor, handle, 0, dataRead);
    if(ret < 0)
    {
      delete[] buffer;
      throw EXCEPTION("Unable to write file to socket!", 500);
    }
    pos += ret;
    dataRead -= ret;

#else

    ret = read(handle, buffer, 8192 * sizeof(char));
    if(ret < 0)
    {
      if(errno == EINTR) continue;
      else throw EXCEPTION("Unable to read from file descriptor!", 500);
    }

    do
    {
      ret = send(me._fileDescriptor, buffer, ret);
      if(ret < 0)
      {
        if(errno == EINTR) continue;
      }

      pos += ret;
      dataRead -= ret;
    } while(ret < 0);

#endif

  }

  delete[] buffer;
  return ret;
}

std::string TcpServerConnection::readline(char end, size_t mx)
{
  std::stringstream str;
  size_t l = 0;
  while(mx == 0 || l++ <= mx)
  {
    char x;
    me.read(x);
    if(x == end) break;
    str << x;
  }
  if(l >= mx && mx != 0)
    throw EXCEPTION("Error. Data delimiter not reached even though max length reached", l);

  return str.str();
}
