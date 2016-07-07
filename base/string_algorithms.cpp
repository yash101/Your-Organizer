#include "string_algorithms.h"

#include <sstream>

#include <openssl/sha.h>

std::vector<std::string> base::splitByFirstDelimiter(std::string str, std::string find)
{
  std::vector<std::string> ret;
  if(str.size() == 0) return ret;
  size_t f = str.find(find);
  if(f == std::string::npos)
  {
    ret.push_back(str);
    return ret;
  }
  else if(f + find.size() >= str.size())
  {
    ret.push_back(str.substr(0, str.size() - find.size()));
    return ret;
  }
  else
  {
    ret.push_back(str.substr(0, f));
    ret.push_back(str.substr(f + find.size(), str.size()));
    return ret;
  }
}

size_t base::replaceAll(std::string& in, std::string find, std::string repl)
{
  if(in.size() == 0) return 0;

  std::string ret;
  ret.reserve(in.size());

  size_t start = 0;
  size_t pos = 0;
  size_t n = 0;

  while((pos = in.find(find, start)) != std::string::npos)
  {
    n++;
    ret += in.substr(start, pos - start);
    ret += repl;
    pos += find.length();
    start = pos;
  }

  ret += in.substr(start);
  in.swap(ret);
  return n;
}
