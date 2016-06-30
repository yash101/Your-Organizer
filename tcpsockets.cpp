#include "tcpsockets.h"
#include "exceptions.h"
#include "automaticmutex.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <thread>
#include <mutex>
#include <string.h>
#include <stdio.h>

#ifndef TCPQUEUESIZE
#define TCPQUEUESIZE 5
#endif

#ifndef SELECT_TIMEOUT_SEC
#define SELECT_TIMEOUT_SEC 1
#endif

static struct timeval select_timeout;

//Include GNU/Linux/UNIX headers for socket programming
#ifndef _WIN32

#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#define SOCKET_VALID(fd) (fd < 0)
#define SOCKET_NOERROR(x) (x >= 0)
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
  select_timeout.tv_sec = SELECT_TIMEOUT_SEC;
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

int sock::deinitializeSockets()
{
  return WSACleanup();
}

//Initialize non-windows sockets (nothing to do)
#else



inline int sock::initializeSockets()
{
  select_timeout.tv_sec = SELECT_TIMEOUT_SEC;
  select_timeout.tv_usec = 0;
  return 0;
}

inline int sock::deinitializeSockets()
{
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
  this->_isServerRunning = false;
  this->_nConnectedClients = 0;
  this->_nMaxConnectedClients = 0;

  this->_timeoutStructure = (void*) new struct timeval;

  this->_timeoutStructureMutex = (MUTEX*) new std::mutex;
  this->_connectedClientsMutex = (MUTEX*) new std::mutex;


  //Create IPC pipe
  int pipes[2];
  int ret = pipe2(pipes, O_NONBLOCK);
  if(ret < 0)
  {
    delete (struct timeval*) this->_timeoutStructure;
    delete (std::mutex*) this->_timeoutStructureMutex;
    delete (std::mutex*) this->_connectedClientsMutex;

    throw EXCEPTION("Unable to create IPC Pipe!", errno);
  }

  this->_signaler_pipe = pipes[0];
  this->_target_pipe = pipes[1];
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

  //Delete the timeout-handling structure
  delete ((struct timeval*) this->_timeoutStructure);

  //Delete the mutexes for the timeout and number of connected clients
  delete ((std::mutex*) this->_timeoutStructureMutex);
  delete ((std::mutex*) this->_connectedClientsMutex);


  /****************************************
   *+-----------------------------------++*
   *|    ^													 	  ||*
   *|   / \														  ||*
   *|  / | \  MOVE TO stop_server				||*
   *| /  .  \													  ||*
   *| ```````													  ||*
   *+===================================++*
   ****************************************/
  //Deallocate any memory used by struct sockaddr_in*'s
  for(size_t i = 0; i < this->_sockets.size(); i++)
  {
    if(this->_sockets[i].address != NULL)
    {
      delete ((struct sockaddr_in*) this->_sockets[i].address);
    }
  }
}


//Turn on the server
sock::SocketServer& sock::SocketServer::startServer()
{
  if(this->_isServerRunning)
    throw EXCEPTION("Server already running!", this->_sockets.size());
  this->_isServerRunning = true;

  for(size_t i = 0; i < this->_sockets.size(); i++)
  {
    sock::ListenerSocket sock = this->_sockets[i];

    //Enable SSL. If fails, mark this socket as no SSL
    if(sock.arguments.SSLEnable)
    {
      sock.SSLContext = (void*) SSL_CTX_new(TLSv1_1_server_method());
      if(sock.SSLContext == NULL)
      {
        ERR_print_errors_fp(stderr);
        fprintf(stderr, "Error creating SSL context. Will disable SSL");
        sock.arguments.SSLEnable = false;
      }
      else
      {
        if(SSL_CTX_use_certificate_file((SSL_CTX*) sock.SSLContext, sock.arguments.privateKeyPath, SSL_FILETYPE_PEM) <= 0)
        {
          ERR_print_errors_fp(stderr);
          abort();
        }

        if(SSL_CTX_use_PrivateKey_file((SSL_CTX*) sock.SSLContext, sock.arguments.privateKeyPath, SSL_FILETYPE_PEM) <= 0)
        {
          ERR_print_errors_fp(stderr);
          abort();
        }
      }
    }

    sock.fd = socket(AF_INET, SOCK_STREAM | O_NONBLOCK, 0);
    if(!SOCKET_NOERROR(sock.fd))
    {
      if(errno == EACCES || errno == EAFNOSUPPORT || errno == EINVAL || errno == EMFILE || errno == ENFILE || errno == ENOBUFS || errno == ENOMEM || errno == EPROTONOSUPPORT)
      {
        for(size_t j = 0; j < i; i++)
        {
#ifndef _WIN32
          close(this->_sockets[i].fd);
#else
          closesocket(this->_sockets[i].fd);
#endif
          delete ((struct sockaddr_in*) this->_sockets[i].address);
        }
        throw EXCEPTION("Unable to create new socket! ERROR!", sock.listeningPort);
      }
    }


    //Set the address structure
    sock.address = (void*) new (std::nothrow) struct sockaddr_in;
    ((struct sockaddr_in*) sock.address)->sin_addr.s_addr = INADDR_ANY;
    ((struct sockaddr_in*) sock.address)->sin_family = AF_INET;
    ((struct sockaddr_in*) sock.address)->sin_port = htons(sock.listeningPort);
    this->_sockets[i] = sock;


    //Set reuseaddr to true
#ifdef _WIN32
    typedef char sso_tp;
#else
    typedef const void sso_tp;
#endif

    int reuseaddr = 1;
    setsockopt(sock.fd, SOL_SOCKET, SO_REUSEADDR, (sso_tp*) &reuseaddr, (socklen_t) sizeof(decltype(reuseaddr)));


    //Bind socket to address
    int ret = bind(
          sock.fd,
          (struct sockaddr*) sock.address,
          sizeof(struct sockaddr_in)
    );

    //Close all connections and exit
    if(!SOCKET_NOERROR(ret))
    {
      for(size_t j = 0; j <= i; i++)
      {
#ifndef _WIN32
        close(this->_sockets[i].fd);
#else
        closesocket(this->_sockets[i].fd);
#endif
        delete ((struct sockaddr_in*) this->_sockets[i].address);
      }
    }


    //Listen for incoming connections
    listen(sock.fd, TCPQUEUESIZE);

    //Run callback for when port is assigned
    this->whenPortIsAssigned(sock.listeningPort);
  }

  this->accepter();
  return (*this);
}


void sock::SocketServer::stopServer()
{
  send(this->_signaler_pipe, "\0", sizeof("\0"), MSG_NOSIGNAL);
}

//Add a port to listen on
sock::SocketServer& sock::SocketServer::addListeningPort(int port, SSLArguments arguments)
{
  if(this->_isServerRunning)
  {
    throw EXCEPTION("Unable to add port! Server is already running!", this->_sockets.size());
    return (*this);
  }

  for(size_t i = 0; i < this->_sockets.size(); i++)
  {
    if(this->_sockets[i].listeningPort == port)
    {
      this->_sockets[i].arguments = arguments;
      return (*this);
    }
  }

  sock::ListenerSocket sock;
  sock.address = (void*) new (std::nothrow) struct sockaddr_in;
  sock.arguments = arguments;
  sock.fd = -1;
  sock.listeningPort = port;

  this->_sockets.push_back(sock);

  return (*this);
}

sock::SocketServer& sock::SocketServer::addListeningPort(int port)
{
  return this->addListeningPort(port, sock::SSLArguments());
}



void sock::SocketServer::whenPortIsAssigned(int port)
{
  fprintf(stdout, "Default server created with port, [%d]\n", port);
}

void sock::SocketServer::incomingConnection(sock::OutConnection* conn)
{
  conn->write("Hello World!\n");
}

static int sizeof_si = sizeof(struct sockaddr_in);

void sock::SocketServer::accepter()
{
  //Create the FD set
  fd_set fds;
  int maxfd = this->_target_pipe;
  FD_ZERO(&fds);
  for(size_t i = 0; i < this->_sockets.size(); i++)
  {
    FD_SET(this->_sockets[i].fd, &fds);
    if(this->_sockets[i].fd > maxfd)
      maxfd = this->_sockets[i].fd;
  }

  FD_SET(this->_target_pipe, &fds);

  while(true)
  {
    //Back up stuff. select gobbles up things
    fd_set descriptors = fds;
    struct timeval timeout = select_timeout;
    int ret = select(maxfd + 1, &descriptors, NULL, NULL, &timeout);

    //Timeout
    if(ret == 0)
      continue;

    else if(!SOCKET_NOERROR(ret))
    {
      if(errno == EINTR || errno == EINVAL || errno == ENOMEM)
      {
        FD_ZERO(&fds);
        break;
      }
      else
      {
        continue;
      }
    }

    for(size_t i = 0; i < this->_sockets.size(); i++)
    {
      //Find ready connection
      if(FD_ISSET(this->_sockets[i].fd, &descriptors))
      {
        //Accept ready connection
        OutConnection* connection = new (std::nothrow) OutConnection;
        if(connection == NULL) break;

        connection->FileDescriptor() = accept(
              this->_sockets[i].fd,
              (struct sockaddr*) connection->LocalAddress(),
              (socklen_t*) &sizeof_si
        );
        if(connection->FileDescriptor() < 0)
        {
          perror("Accept failed");
          delete connection;
          break;
        }

        connection->Ssl() = NULL;
        if(this->_sockets[i].arguments.SSLEnable)
        {
          connection->Ssl() = (void*) SSL_new((SSL_CTX*) this->_sockets[i].SSLContext);
          SSL_set_fd((SSL*) connection->Ssl(), this->_sockets[i].fd);
        }

        std::thread(&sock::SocketServer::runner, this, connection).detach();
      }
    }

    if(FD_ISSET(this->_target_pipe, &descriptors))
    {
      fprintf(stdout, "Exiting...\n");
      for(size_t i = 0; i < this->_sockets.size(); i++)
      {
        if(this->_sockets[i].address == NULL)
        {
          delete (struct sockaddr_in*) this->_sockets[i].address;
        }

        close(this->_sockets[i].fd);
        if(this->_sockets[i].arguments.SSLEnable == true)
        {
          SSL_CTX_free((SSL_CTX*) this->_sockets[i].SSLContext);
        }
      }
    }
  }
}

void sock::SocketServer::runner(void* connection)
{
  try
  {
    this->incomingConnection((sock::OutConnection*) connection);
  }
  catch(std::exception&)
  {}

  sock::OutConnection* conn = (sock::OutConnection*) connection;
  if(conn->Ssl() != NULL)
  {
    SSL_free((SSL*) conn->Ssl());
  }
  delete (sock::OutConnection*) connection;
}


void sock::initSSL()
{
  SSL_load_error_strings();
  SSL_library_init();
  OpenSSL_add_all_algorithms();
}

void sock::destSSL()
{
  ERR_free_strings();
  EVP_cleanup();
}

void sock::initialize()
{
  sock::initializeSockets();
  sock::initSSL();
}

void sock::deinitialize()
{
  sock::destSSL();
  sock::deinitializeSockets();
}

void sock::SocketServer::shutdownSSL(void* SSL_var)
{
  SSL_shutdown((SSL*) SSL_var);
  SSL_free((SSL*) SSL_var);
}


  /****************************************
   *+-----------------------------------++*
   *|    ^													 	  ||*
   *|   / \														  ||*
   *|  / | \  !!!IMPLEMENT!!! IMPORTANT!||*
   *| /  .  \													  ||*
   *| ```````
   *+===================================++*
   ****************************************/
