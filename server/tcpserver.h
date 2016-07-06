#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <string>
#include <boost/thread.hpp>

//Windows does not have ssize_t
#ifdef _WIN32
typedef signed size_t ssize_t;
#endif

#ifdef _WIN32

typedef unsigned int SOCK_HANDLE;
typedef unsigned int SOCK_RET;
typedef int SOCK_IO_RET;

#define SOCKET_INITIALIZER INVALID_SOCKET
#define FD_VALID(fd) (fd != INVALID_SOCKET)
#define SOCKET_ERROR(ret) (ret == SOCKET_ERROR)

#else

typedef int SOCK_HANDLE;
typedef int SOCK_RET;
typedef ssize_t SOCK_IO_RET;

#define SOCKET_INITIALIZER -1
#define FD_VALID(fd) (fd >= 0)
#define SOCKET_ERROR(ret) (ret < 0)
#endif

#ifndef CONNECTION_ALLOC_ATTEMPT_WAITOUT
#define CONNECTION_ALLOC_ATTEMPT_WAITOUT 10   //milliseconds
#endif

namespace srv
{
  class TcpServer;
  class TcpServerConnection;
  struct ConnectionStatistics;

  void initializeSockets();
  void uninitializeSockets();

  namespace error
  {
    const static int ServerAlreadyRunning = -1;
    const static int SocketCreationFailed = -2;
    const static int SocketBindFailed = -3;
    const static int SocketReceiveFailed = -4;
  }

  struct ConnectionStatistics
  {
    size_t maximum_simultaneous_clients;
    size_t maximum_session_length;
    size_t minimum_session_length;
    size_t mean_session_length;					//May not actually implement due to massive requirements
  };

  class TcpServerConnection
  {
    friend class TcpServer;

  private:

    SOCK_HANDLE _fileDescriptor;
    void* _addressStructure;
    void* _timeoutStructure;
    TcpServer* _listeningServer;
    bool _isClientCounterActive;

    SOCK_IO_RET read_internal(void* buffer, size_t sz, int flags);
    SOCK_IO_RET write_internal(void* buffer, size_t sz, int flags);

  public:

    TcpServerConnection();
    ~TcpServerConnection();

    SOCK_IO_RET read(char& ch);
    SOCK_IO_RET read(void* buffer, size_t len);
    SOCK_IO_RET read(std::string& str, size_t mxlen);

    SOCK_IO_RET write(char ch);
    SOCK_IO_RET write(void* buffer, size_t len);
    SOCK_IO_RET write(char* str);
    SOCK_IO_RET write(const char* str);
    SOCK_IO_RET write(std::string str);
    SOCK_IO_RET write_fd(SOCK_HANDLE handle, size_t dataRead);

    std::string readline(char end, size_t mx);


    inline SOCK_HANDLE getNativeHandle()
    {
      return (*this)._fileDescriptor;
    }
  };

  class TcpServer
  {
    friend class TcpServerConnection;

  private:

    SOCK_HANDLE _fileDescriptor;
    int _listeningPort;
    bool _isServerRunning;

    void* _addressStructure;
    void* _timeoutStructure;
    boost::mutex _timeoutMutex;

    size_t _simultaneousClients;
    size_t _maxSimultaneousClients;
    boost::mutex _connectedClientsMutex;

    int _tcpConnectionQueue;

    int acceptor();
    void workerProxy(TcpServerConnection* connection);

    void initializeAllVariables();
    void destroyAllVariables();

  protected:

    virtual void worker(TcpServerConnection& connection);

  public:

    TcpServer();
    virtual ~TcpServer();

    int startServer();

    TcpServer& setPort(int port);
    int getPort();

    TcpServer& setMaximumConcurrentClients(size_t num);
    size_t getMaximumConcurrentClients();

    TcpServer& setTcpQueueSize(int qs);
    int getTcpQueueSize();

    TcpServer& setSocketTimeout(long usec, long sec);
    TcpServer& setSocketTimeoutMicroseconds(long usec);
    TcpServer& setSocketTimeoutSeconds(long sec);
    long getSocketTimeoutMicroseconds();
    long getSocketTimeoutSeconds();

    size_t getNumberCurrentConcurrentClients();
    ConnectionStatistics getStatistics();

    bool isServerRunning();
  };
}
#endif // TCPSERVER_H
