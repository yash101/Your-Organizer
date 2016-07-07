#ifndef ORGANIZERWEBSERVICE_H
#define ORGANIZERWEBSERVICE_H
#include "../server/httpserver.h"

namespace core
{
  class MainServer : public http::HttpServer
  {
  private:
  protected:

    void requestHandler(http::HttpSession& session);

  public:
  };
}

#endif // ORGANIZERWEBSERVICE_H
