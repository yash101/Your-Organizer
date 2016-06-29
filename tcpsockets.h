#ifndef TCPSOCKETS_H
#define TCPSOCKETS_H

#include "tcpconnection.h"

#include <map>

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

  struct SSLArguments
  {
  public:
    const char* privateKeyPath;
    bool SSLEnable;

    SSLArguments();
    SSLArguments(const char* priKey);
  };

  class OutConnection : public sock::Connection
  {
  public:
    inline void* TimeoutStructure() { return this->_timeout; }
    inline void* LocalAddress() { return this->_address; }
    inline void* RemoteAddress() { return this->_remoteAddress; }
    inline int& FileDescriptor() { return this->_fd; }
  };

  class SocketServer
  {
  private:

    int _ictr = 0;
    MUTEX* _fileDescriptorsMutex;
    std::map<int, SOCK_HANDLE> _fileDescriptors;

    MUTEX* _addressesMutex;
    std::map<int, void*> _addresses;

    MUTEX* _listeningPortsMutex;
    std::map<int, int> _listeningPorts;

    MUTEX* _SSLVarsMutex;
    std::map<int, SSLArguments> _SSLVars;


    MUTEX* _timeoutStructureMutex;
    void* _timeoutStructure;

    MUTEX* _connectedClientsMutex;
    size_t _nConnectedClients;
    size_t _nMaxConnectedClients;

    void prepareSocket(int key, int port);

    void accepter();
    void runner(void* connection);

    void initializeVariables();

  protected:

    virtual void whenPortIsAssigned(int port);

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
  };
}

#endif // TCPSOCKETS_H
