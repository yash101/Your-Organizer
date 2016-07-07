#ifndef HTTPREQUESTHANDLER_H
#define HTTPREQUESTHANDLER_H
#include "webserver.h"
#include <map>
#include <string>

namespace yo
{
  const static char mm_regex = 0x2;
  const static char mm_compare = 0x4;

  class HttpAction;
  class HostHandler;

  //Holds all the hostnames [globally]
  extern std::multimap<size_t, HostHandler> hostnames;
  void httpRequestHandler(http::HttpSession& session);

  //Holds information about each HTTP action
  class HttpAction
  {
  public:
    HttpAction();

    std::string matcher;
    char matching_method;
    virtual bool operator()(http::HttpSession& session);
  };

  //Selects and runs an HTTP action
  class HostHandler
  {
  public:
    HostHandler();
    bool operator()(http::HttpSession& session);

    std::string matcher;
    char matching_method;

    std::multimap<size_t, HttpAction> actions;
  };
}
#endif // HTTPREQUESTHANDLER_H
