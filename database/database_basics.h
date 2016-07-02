#ifndef DATABASE_BASICS_H
#define DATABASE_BASICS_H

#include <postgresql/libpq-fe.h>

#ifndef MAXCONNECTIONSPOLLWAIT
#define MAXCONNECTIONSPOLLWAIT 100
#endif

namespace db
{
  void init();
  PGconn* acquireNewDatabaseConnection();
  void releaseDatabaseConnection(PGconn* connection);

  class databaseHandle
  {
  private:
    PGconn* connection;

  public:
    databaseHandle();
    ~databaseHandle();

    PGconn* getConnection();
    PGconn* operator()();
    PGconn* operator->();
  };
}

#endif // DATABASE_BASICS_H
