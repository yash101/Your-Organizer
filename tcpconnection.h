#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H
#include <stddef.h>

#ifndef _WIN32
#include <sys/types.h>
#endif

namespace sock
{
#ifdef _WIN32
typedef int SOCK_RW_RET;
#else
typedef ssize_t SOCK_RW_RET;
#endif

  class Connection
  {
  protected:
    int _fd;
    void* _address;
    void* _remoteAddress;
    void* _timeout;
    void* _ssl;

    SOCK_RW_RET read_int(void* buffer, size_t len, int flags);
    SOCK_RW_RET write_int(void* buffer, size_t len, int flags);

  public:
    Connection();
    virtual ~Connection();

    SOCK_RW_RET read(void* buffer, size_t bufferLength);
    SOCK_RW_RET read(char& ch);
    SOCK_RW_RET write(void* buffer, size_t bufferLength);
    SOCK_RW_RET write(char* str);
    SOCK_RW_RET write(const char* str);
    SOCK_RW_RET write(char ch);
  };
}

#endif // TCPCONNECTION_H
