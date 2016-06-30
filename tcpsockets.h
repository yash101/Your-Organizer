#ifndef TCPSOCKETS_H
#define TCPSOCKETS_H

#include "tcpconnection.h"

#include <vector>

//Socket file descriptors are unsigned on Windows[32]
//To ensure bug-free!
#ifdef _WIN32
typedef unsigned int SOCK_HANDLE;
#else
typedef int SOCK_HANDLE;
#endif

namespace sock
{
  typedef void MUTEX;

  int initializeSockets();
  int deinitializeSockets();
  void initSSL();
  void destSSL();
  void initialize();
  void deinitialize();

  struct SSLArguments
  {
  public:
    const char* privateKeyPath;
    bool SSLEnable;

    SSLArguments();
    SSLArguments(const char* priKey);
  };

  struct ListenerSocket
  {
  public:
    SSLArguments arguments;
    int fd;
    void* address;
    int listeningPort;
    void* SSLContext;
  };

  class OutConnection : public sock::Connection
  {
  public:
    inline void* TimeoutStructure() { return this->_timeout; }
    inline void* LocalAddress() { return this->_address; }
    inline void* RemoteAddress() { return this->_remoteAddress; }
    inline int& FileDescriptor() { return this->_fd; }
    inline void*& Ssl() { return this->_ssl; }
  };

  class SocketServer
  {
  private:

    bool _isServerRunning;
    std::vector<ListenerSocket> _sockets;

    MUTEX* _timeoutStructureMutex;
    void* _timeoutStructure;

    MUTEX* _connectedClientsMutex;
    size_t _nConnectedClients;
    size_t _nMaxConnectedClients;

    SOCK_HANDLE _signaler_pipe;
    SOCK_HANDLE _target_pipe;

    void accepter();
    void runner(void* connection);

    void initializeVariables();

    void shutdownSSL(void* SSL_Var);

  protected:

    virtual void whenPortIsAssigned(int port);
    virtual void incomingConnection(OutConnection* connection);

  public:

    SocketServer();
    virtual ~SocketServer();

    SocketServer& startServer();
    SocketServer& addListeningPort(int port, SSLArguments arguments);
    SocketServer& addListeningPort(int port);

    SocketServer& setSocketTimeout(int usec, int sec);
    int getTimeoutMicroseconds();
    int getTimeoutSeconds();

    SocketServer& setMaximumConnectedClients(size_t max);
    size_t getMaximumConnectedClients();
    size_t getNumberConnectedClients();

    SocketServer& setTCPQueueSize(int n);
    int getTCPQueueSize();

    void stopServer();
  };
}

#endif // TCPSOCKETS_H
