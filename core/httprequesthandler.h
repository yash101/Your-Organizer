#ifndef HTTPREQUESTHANDLER_H
#define HTTPREQUESTHANDLER_H
#include "webserver.h"
#include <map>
#include <string>

namespace yo
{
  const static char mm_regex = 0x2;
  const static char mm_compare = 0x4;

  typedef bool(*ActionFunction)(http::HttpSession&, void* data);

  class HttpAction;
  class HostHandler;

  //Holds all the hostnames [globally]
  extern std::multimap<size_t, HostHandler> hostnames;
  void httpRequestHandler(http::HttpSession& session);

  //Holds information about each HTTP action
  class HttpAction
  {
    friend class HostHandler;

  protected:

    virtual bool operator()(http::HttpSession& session);

  public:
    ActionFunction function_pointer;

    HttpAction();

    std::string matcher;
    char matching_method;
    void* data;
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

  bool static_handler(http::HttpSession& session, void* data);
}
#endif // HTTPREQUESTHANDLER_H
