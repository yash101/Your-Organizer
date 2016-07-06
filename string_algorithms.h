#ifndef STRING_ALGORITHMS_H
#define STRING_ALGORITHMS_H

#include <string>
#include <vector>

namespace base
{
  std::vector<std::string> splitByFirstDelimiter(std::string str, std::string find);
  std::string SHA1Hash(std::string in);
}

#endif // STRING_ALGORITHMS_H
