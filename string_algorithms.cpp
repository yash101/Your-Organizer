#include "string_algorithms.h"

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
