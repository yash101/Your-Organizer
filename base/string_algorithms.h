#ifndef STRING_ALGORITHMS_H
#define STRING_ALGORITHMS_H

#include <string>
#include <vector>

namespace base
{
  std::vector<std::string> splitByFirstDelimiter(std::string str, std::string find);
  size_t replaceAll(std::string& in, std::string find, std::string replace);
}

#endif // STRING_ALGORITHMS_H
