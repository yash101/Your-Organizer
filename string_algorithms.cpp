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

std::string base::SHA1Hash(std::string in)
{
  unsigned char hash[SHA_DIGEST_LENGTH + 1];
  hash[SHA_DIGEST_LENGTH] = '\0';
  SHA1(in.c_str(), in.size(), hash);
  return std::string(hash);
}
