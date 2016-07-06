#include "tcpserver.h"
#include "../base/exceptions.h"
#include "../base/automaticmutex.h"

#include <netinet/in.h>

#define me (*this)

using namespace srv;

#define ADDR(x) ((struct sockaddr_in6*) x)
#ifdef _WIN32
typedef char* sso_tp;
#else
typedef const void* sso_tp;
#endif

static const int reuseaddr_1 = 1;

int TcpServer::startServer()
{
  //Initialize
  initializeSockets();

  //Check if server is already running
  if(me._isServerRunning)
    return error::ServerAlreadyRunning;
  me._isServerRunning = true;

  //Create/check socket
  me._fileDescriptor = socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP);
  if(!FD_VALID(me._fileDescriptor))
  {
    me._fileDescriptor = SOCKET_INITIALIZER;
    return error::SocketCreationFailed;
  }

  //Fill out struct sockaddr_in6
  ADDR(me._addressStructure)->sin6_family = AF_INET6;
  ADDR(me._addressStructure)->sin6_addr = in6addr_any;
  ADDR(me._addressStructure)->sin6_port = htons(me._listeningPort);
  ADDR(me._addressStructure)->sin6_flowinfo = 0;

  //Set SO_REUSEADDR as true. Saves the headaches later with the TIME_WAIT shenanigans!
  setsockopt(me._fileDescriptor, SOL_SOCKET, SO_REUSEADDR, (sso_tp*) &reuseaddr_1, sizeof(reuseaddr_1));

  //Bind the sockaddr_in6 structure to the file descriptor
  SOCK_RET ret = bind(me._fileDescriptor, (struct sockaddr*) me._addressStructure, sizeof(struct sockaddr_in6));
  if(SOCKET_ERROR(ret))
  {
    perror("An error ocurred while attempting to bind socket: ");
    close(me._fileDescriptor);
    me._fileDescriptor = SOCKET_INITIALIZER;
    return error::SocketBindFailed;
  }

  //Listen for incoming connections
  listen(me._fileDescriptor, me._tcpConnectionQueue);

  return me.acceptor();
}

static const int sizeof_struct_sockaddr_in6 = sizeof(struct sockaddr_in6);

int TcpServer::acceptor()
{
  //Loop indefinitely
  while(true)
  {
    //Allocate space for the new and shiny TcpServerConnection object
    //Must be explicitly freed!
    TcpServerConnection* connection = new TcpServerConnection;
    if(connection == NULL)
    {
      boost::this_thread::sleep_for(boost::chrono::milliseconds(CONNECTION_ALLOC_ATTEMPT_WAITOUT));
      continue;
    }

    //Wait until we are under the maximum concurrent request quota
    base::AutoMutex<boost::mutex> mtx(me._connectedClientsMutex);
    mtx.unlock();
    while(true)
    {
      size_t nmc = 0;
      size_t nc = 0;

      mtx.lock();
      nmc = me._maxSimultaneousClients;
      nc = me._simultaneousClients;
      mtx.unlock();

      if(nmc == 0 || nc < nmc)
      {
        mtx.lock();
        me._simultaneousClients++;
        mtx.unlock();
        break;
      }
    }

    //Set a reference to this server object
    connection->_listeningServer = this;
    //Accept the new connection
    connection->_fileDescriptor = accept(me._fileDescriptor, NULL, NULL);
    if(!FD_VALID(connection->_fileDescriptor))
    {
      perror("Error accepting new connection: ");
      continue;
    }
    else
    {
      base::AutoMutex<boost::mutex> timeoutMutex(me._timeoutMutex);
      memcpy(connection->_timeoutStructure, me._timeoutStructure, sizeof(struct timeval));
      timeoutMutex.unlock();

      setsockopt(connection->_fileDescriptor, SOL_SOCKET, SO_SNDTIMEO, (sso_tp*) connection->_timeoutStructure, (socklen_t) sizeof(struct timeval));
      setsockopt(connection->_fileDescriptor, SOL_SOCKET, SO_RCVTIMEO, (sso_tp*) connection->_timeoutStructure, (socklen_t) sizeof(struct timeval));

      getpeername(connection->_fileDescriptor, (struct sockaddr*) connection->_addressStructure, (socklen_t*) &sizeof_struct_sockaddr_in6);
    }

    //Create and detach the new thread
    boost::thread t(boost::bind(&TcpServer::workerProxy, this, connection));
    t.detach();
  }
}

void TcpServer::workerProxy(TcpServerConnection* connection)
{
  try
  {
    me.worker(*connection);
  }
  catch(std::exception& e)
  {
    fprintf(stderr, "An error ocurred: [%s]\n", e.what());
  }
  delete connection;
}

/*		This is the function that shall be overridden when developing with the server. */
void TcpServer::worker(TcpServerConnection& connection)
{
  connection.write("Hello World!");
}
