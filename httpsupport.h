#ifndef HTTPSUPPORT_H
#define HTTPSUPPORT_H
#include <string>
namespace http
{
  std::string encodeURI(std::string in);
  std::string decodeURI(std::string in);
}
#endif // HTTPSUPPORT_H
