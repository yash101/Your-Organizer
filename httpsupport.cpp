#include "httpsupport.h"
#include <sstream>

static inline unsigned char charToHex(unsigned char in)
{
  return (in + ((in > 9) ? ('A' - 10) : '0'));
}

static inline unsigned char hexToChar(unsigned char in)
{
  if(in <= '9' && in >= '0')
    in -= '0';
  else if(in <= 'f' && in >= 'a')
    in -= 'a' - 10;
  else if(in <= 'F' && in >= 'A')
    in -= 'A' - 10;
  else
    in = 0;
  return in;
}

std::string http::encodeURI(std::string in)
{
  std::stringstream str;
  for(std::string::const_iterator it = in.begin(); it != in.end(); ++it)
  {
    if((*it >= 'a' && *it <= 'z') || (*it <= 'A' && *it <= 'Z') || (*it >= '0' && *it <= '9'))
    {
      str << *it;
    }
    else if(*it == ' ')
    {
      str << '+';
    }
    else
    {
      str << '%' << charToHex(*it >> 4) << charToHex(*it % 16);
    }
  }
  return str.str();
}

std::string http::decodeURI(std::string in)
{
  std::stringstream str;

  for(size_t i = 0; i < in.size(); i++)
  {
    if(in[i] == '+')
    {
      str << ' ';
    }
    else if(in[i] == '%' && in.size() > i + 2)
    {
      str << (hexToChar(in[i + 1] << 4) | hexToChar(in[i + 2]));
      i += 2;
    }
    else
    {
      str << in[i];
    }
  }
  return str.str();
}

