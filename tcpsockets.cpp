#include "tcpsockets.h"
#include "exceptions.h"
#include "automaticmutex.h"

#include <thread>
#include <mutex>
#include <string.h>
#include <stdio.h>

#ifndef TCPQUEUESIZE
#define TCPQUEUESIZE 5
#endif

#ifndef ACCEPT_TIMEOUT_SEC
#define ACCEPT_TIMEOUT_SEC 1
#endif

static struct timeval select_timeout;

//Include GNU/Linux/UNIX headers for socket programming
#ifndef _WIN32

#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

#define SOCKET_VALID(fd) (fd < 0)
#define SOCKET_NOERROR(x) (x > 0)
#define FD_INITIALIZER -1

//Include Windows headers for socket programming
#else

#pragma comment(lib, "Ws2_32.lib")
#include <Windows.h>

static WSADATA WsaData;
static WORD WsaVersionRequested = NULL;
static int WsaError = 0;
static bool WsaInitialized = false;

#define SOCKET_VALID(fd) (fd != INVALID_SOCKET)
#define SOCKET_NOERROR(x) (x != SOCKET_ERROR)
#define FD_INITIALIZER INVALID_SOCKET

#endif


//Initialize WinSock
#ifdef _WIN32

int sock::initializeSockets()
{
  select_timeout.tv_sec = ACCEPT_TIMEOUT_SEC;
  select_timeout.tv_usec = 0;
  if(!WsaInitialized)
  {
    WsaVersionRequested = MAKEWORD(2, 2);
    WsaError = WSAStartup(WsaVersionRequested, WsaData);

    if(WsaError == 0)
      WsaInitialized = true;
  }

  return WsaError;
}

//Initialize non-windows sockets (nothing to do)
#else

inline int sock::initializeSockets()
{
  select_timeout.tv_sec = ACCEPT_TIMEOUT_SEC;
  select_timeout.tv_usec = 0;
  return 0;
}

#endif



//Structure to set up SSL - Constructors
sock::SSLArguments::SSLArguments() :
  privateKeyPath(NULL),
  SSLEnable(false)
{
}
sock::SSLArguments::SSLArguments(const char* priKey) :
  privateKeyPath(priKey),
  SSLEnable((priKey == NULL) ? false : true)
{
}


//Initialize variables
//May throw std::bad_alloc in case of failure to allocate any of the
//mutexes or timeval structures
void sock::SocketServer::initializeVariables()
{
  this->_ictr = 0;
  this->_nConnectedClients = 0;
  this->_nMaxConnectedClients = 0;

  this->_timeoutStructure = (void*) new struct timeval;

  this->_fileDescriptorsMutex = (MUTEX*) new std::mutex;
  this->_addressesMutex = (MUTEX*) new std::mutex;
  this->_listeningPortsMutex = (MUTEX*) new std::mutex;
  this->_SSLVarsMutex = (MUTEX*) new std::mutex;
  this->_timeoutStructureMutex = (MUTEX*) new std::mutex;
  this->_connectedClientsMutex = (MUTEX*) new std::mutex;
}

//Initialize SocketServer class
sock::SocketServer::SocketServer()
{
  this->initializeVariables();
}

//Destroy the SocketServer
sock::SocketServer::~SocketServer()
{
  //Tons of memory management :)
  //Kinda hate my life by this time, but whatever :)
  delete ((struct timeval*) this->_timeoutStructure);

  delete ((std::mutex*) this->_fileDescriptorsMutex);
  delete ((std::mutex*) this->_addressesMutex);
  delete ((std::mutex*) this->_listeningPortsMutex);
  delete ((std::mutex*) this->_SSLVarsMutex);
  delete ((std::mutex*) this->_timeoutStructureMutex);
  delete ((std::mutex*) this->_connectedClientsMutex);

  for(std::map<int, void*>::const_iterator it = this->_addresses.begin(); it != this->_addresses.end(); ++it)
  {
    struct sockaddr_in* s = (struct sockaddr_in*) it->second;
    delete s;
  }
}


//Turn on the server
sock::SocketServer& sock::SocketServer::startServer()
{
  //Check to make sure there is at least one port we would like to listen on
  if(this->_listeningPorts.empty())
  {
    throw EXCEPTION("Attempting to start server with no listening ports!", 0);
    return (*this);
  }

  throw EXCEPTION("NOT IMPLEMENTED YET :(", -1);
}


//Add a port to listen on
sock::SocketServer& sock::SocketServer::addListeningPort(int port, SSLArguments arguments)
{
  //Check to make sure we do not have a port zero
  if(port == 0)
  {
    throw EXCEPTION("Port 0 is not supported by this server!", 0);
  }

  //Figure out what key to put this under
  int key = -1;
  base::AutoMutex<std::mutex> listPorts(this->_listeningPortsMutex);
  listPorts.lock();
  for(std::map<int, int>::const_iterator it = this->_listeningPorts.begin(); it != this->_listeningPorts.end(); it++)
  {
    if(it->second == port)
    {
      key = it->first;
      break;
    }
  }
  listPorts.unlock();

  if(key < 0)
  {
    //Set the key
    this->_ictr++;
    key = this->_ictr;
    //Set the port [database]
    listPorts.lock();
    this->_listeningPorts[key] = port;
    listPorts.unlock();
  }

  //Set the SSL Variables
  base::AutoMutex<std::mutex> SSLVars(this->_SSLVarsMutex);
  SSLVars.lock();
  this->_SSLVars[key] = arguments;
  SSLVars.unlock();

  //Prepare the socket to begin accepting connections
  if(key == this->_ictr)
    prepareSocket(key, port);

  return (*this);
}

void sock::SocketServer::prepareSocket(int key, int port)
{
  //Create socket and save value
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  base::AutoMutex<std::mutex> fdMutex(this->_fileDescriptorsMutex);
  _fileDescriptors[key] = fd;
  fdMutex.unlock();



  //Create and fill out an address structure
  struct sockaddr_in* addr = NULL;
  try
  {
    addr = new struct sockaddr_in;
    memset((void*) addr, 0, sizeof(struct sockaddr_in));
  }
  catch(std::bad_alloc& e)
  {
    close(fd);
    fdMutex.lock();
    this->_fileDescriptors.erase(key);
    fdMutex.unlock();
    base::AutoMutex<std::mutex> listMtx(this->_listeningPortsMutex);
    this->_listeningPorts.erase(key);
    listMtx.unlock();
    base::AutoMutex<std::mutex> ssmtx(this->_SSLVarsMutex);
    this->_SSLVars.erase(key);
    ssmtx.unlock();

    this->_ictr--;

    throw EXCEPTION("Unable to allocate memory!", -1);
  }

  addr->sin_addr.s_addr = INADDR_ANY;
  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);
  base::AutoMutex<std::mutex> addressMutex(this->_addressesMutex);
  this->_addresses[key] = (void*) addr;
  addressMutex.unlock();



  //Set SO_REUSEADDR to true; Makes for painless debugging/etc.
#ifdef _WIN32
typedef char sso_tp;
#else
typedef const void sso_tp;
#endif

  int reuseAddress = 1;
  if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (sso_tp*) &reuseAddress, sizeof(int)) < 0)
  {
    fprintf(stderr, "Unable to set SO_REUSEADDR to socket; PORT: [%d]; FD: [%d]\n", port, fd);
  }

  //Bind the address structure to the socket
  int ret = bind(fd, (struct sockaddr*) addr, sizeof(struct sockaddr_in));
  if(ret < 0)
  {
    close(fd);
    fdMutex.lock();
    this->_fileDescriptors.erase(key);
    fdMutex.unlock();
    base::AutoMutex<std::mutex> listMtx(this->_listeningPortsMutex);
    this->_listeningPorts.erase(key);
    listMtx.unlock();
    base::AutoMutex<std::mutex> ssmtx(this->_SSLVarsMutex);
    this->_SSLVars.erase(key);
    ssmtx.unlock();

    this->_ictr--;
    throw EXCEPTION("Unable to bind socket to address/port. Please check to make sure that the port is open, not in use, and allowed by your firewall!", ret);
  }

  if(!SOCKET_NOERROR(listen(fd, TCPQUEUESIZE)))
  {
    close(fd);
    fdMutex.lock();
    this->_fileDescriptors.erase(key);
    fdMutex.unlock();
    base::AutoMutex<std::mutex> listMtx(this->_listeningPortsMutex);
    this->_listeningPorts.erase(key);
    listMtx.unlock();
    base::AutoMutex<std::mutex> ssmtx(this->_SSLVarsMutex);
    this->_SSLVars.erase(key);
    ssmtx.unlock();

    this->_ictr--;

    throw EXCEPTION("Unable to listen to socket! FATAL!", errno);
  }
}



void sock::SocketServer::whenPortIsAssigned(int port)
{
  fprintf(stdout, "Default server created with port, [%d]\n", port);
}



void sock::SocketServer::accepter()
{
  fd_set descriptors;

  base::AutoMutex<std::mutex> fdmtx(this->_fileDescriptorsMutex);
  std::map<int, int> lmap = this->_fileDescriptors;
  fdmtx.unlock();

  FD_ZERO(&descriptors);

  for(std::map<int, int>::const_iterator it = lmap.begin(); it != lmap.end(); ++it)
  {
    FD_SET(it->second, &descriptors);
  }

  struct timeval timeout;

  while(true)
  {
    //Store all the sockets we are listening on
    fd_set desc = descriptors;
    timeout = select_timeout;

    //Wait for one of the sockets to get ready
    sock::SOCK_RW_RET ret = select(lmap.size(), &desc, NULL, NULL, &timeout);
    if(ret <= 0)
    {
      //Check for any errors or timeouts. EINTR or timeout mean that we would like to rescan for
      //new ports
      if(errno == EINTR || ret == 0)
      {
        fdmtx.lock();
        bool x = lmap == this->_fileDescriptors;
        lmap = this->_fileDescriptors;
        fdmtx.unlock();

        if(lmap.size() == 0 || lmap.empty())
        {
          FD_ZERO(&descriptors);
          return;
        }

        if(!x)
        {
          lmap.clear();
          fdmtx.lock();
          lmap = this->_fileDescriptors;
          fdmtx.unlock();
          FD_ZERO(&descriptors);

          for(std::map<int, int>::const_iterator it = lmap.begin(); it != lmap.end(); ++it)
          {
            FD_SET(it->second, &descriptors);
          }
        }
      }
      else //Handle some other [possibly fatal] error! :(
      {
        /****************************************
         *+-----------------------------------++*
         *|    ^													 	  ||*
         *|   / \														  ||*
         *|  / | \  !!!IMPLEMENT!!! IMPORTANT!||*
         *| /  .  \													  ||*
         *| ```````
         *+===================================++*
         ****************************************/
      }
    }
    else
    {
      for(std::map<int, int>::const_iterator it = lmap.begin(); it != lmap.end(); ++it)
      {
        if(FD_ISSET(it->second, &desc))
        {
          base::AutoMutex<std::mutex> nmc_m(this->_connectedClientsMutex);
          while(true)
          {
            nmc_m.lock();
            size_t nmc = this->_nMaxConnectedClients;
            size_t cc = this->_nConnectedClients;
            nmc_m.unlock();

            if(nmc == 0 || cc < nmc) break;
          }

          nmc_m.lock();
          this->_nConnectedClients++;
          nmc_m.unlock();

          sock::OutConnection* conn;
          while(true)
          {
            try
            {
              conn = new sock::OutConnection;
              break;
            }
            catch(std::bad_alloc&)
            {
            }
          }

          int ss = sizeof(struct sockaddr_in);
          conn->FileDescriptor() = accept(it->second, (struct sockaddr*) conn->LocalAddress(), (socklen_t*) &ss);
          if(conn->FileDescriptor() < 0)
          {
            if(errno == EBADF || errno == ECONNABORTED || errno == EINVAL)
            {
              delete conn;
              FD_ZERO(&descriptors);
              return;
            }

            if(errno == EAGAIN || errno == EFAULT || errno == EWOULDBLOCK || errno == EMFILE || errno == ENFILE || errno == ENOBUFS || errno == ENOMEM || errno == EOPNOTSUPP || errno == EPROTO || errno == EPERM)
            {
              delete conn;
              break;
            }
          }

          break;
        }
      }
    }
  }
}
