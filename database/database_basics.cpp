#include "database_basics.h"
#include "config.h"
#include "automaticmutex.h"

#include <boost/thread.hpp>
#include <vector>
#include <stdio.h>

#include <postgresql/libpq-fe.h>

static size_t connectionsActive = 0;
static size_t maxActiveConnections = 0;
static boost::mutex connectionsActiveMutex;
static std::string dbname = "";
static std::string dbhost = "";
static std::string dbport = "";
static std::string duname = "";
static std::string passwd = "";

PGconn* db::acquireNewDatabaseConnection()
{
  size_t ca = 0;
  size_t ma = 0;
  base::AutoMutex<boost::mutex> mutex(connectionsActiveMutex);

  while(true)
  {
    mutex.lock();
    ca = connectionsActive;
    ma = maxActiveConnections;
    mutex.unlock();

    if(ma == 0 || ca <= ma)
      break;
    else
      boost::this_thread::sleep_for(boost::chrono::milliseconds(MAXCONNECTIONSPOLLWAIT));
  }

  mutex.lock();
  ca++;
  mutex.unlock();

  PGconn* ret = PQsetdbLogin(
        dbhost.c_str(),
        dbport.c_str(),
        NULL,
        NULL,
        dbname.c_str(),
        duname.c_str(),
        passwd.c_str()
  );
  if(PQstatus(ret) == CONNECTION_BAD)
  {
    fprintf(stderr, "Unable to connect to the database! FATAL!\n");
    PQfinish(ret);
    return NULL;
  }
  return ret;
}

void db::releaseDatabaseConnection(PGconn* connection)
{
  if(connection != NULL)
  {
    PQfinish(connection);
    base::AutoMutex<boost::mutex> mutex(connectionsActiveMutex);
    connectionsActive--;
    mutex.unlock();
  }
}

void db::init()
{
  maxActiveConnections = conf::getConfigInt("database settings maximum_concurrent_connections");
  dbname = conf::getConfigString("database authentication database_name");
  dbhost = conf::getConfigString("database authentication host");
  dbport = conf::getConfigString("database authentication port");
  duname = conf::getConfigString("database authentication username");
  passwd = conf::getConfigString("database authentication password");
}

void dbConfReload()
{
  size_t ret = conf::getConfigInt("database settings maxconcurrentconnections");
  base::AutoMutex<boost::mutex> mtx(connectionsActiveMutex);
  maxActiveConnections = ret;
  mtx.unlock();
}

db::databaseHandle::databaseHandle()
{
  connection = db::acquireNewDatabaseConnection();
}

db::databaseHandle::~databaseHandle()
{
  db::releaseDatabaseConnection(connection);
  connection = NULL;
}

PGconn* db::databaseHandle::getConnection()
{
  return connection;
}

PGconn* db::databaseHandle::operator()()
{
  return connection;
}

PGconn* db::databaseHandle::operator->()
{
  return connection;
}
