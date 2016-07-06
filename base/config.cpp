#include "config.h"
#include <boost/algorithm/string.hpp>

#include <vector>

tinyxml2::XMLDocument conf::config;
static bool inited = false;

void conf::init()
{
  if(inited) return;
  inited = true;
  config.LoadFile(CONFIGURATION_FILE_LOCATION);
}

std::string conf::getConfigString(std::string path)
{
  std::vector<std::string> bpath;
  boost::split(bpath, path, boost::is_any_of(" "));

  if(bpath.size() == 0 || path.size() == 0)
    return "";

  tinyxml2::XMLElement* element = config.FirstChildElement(bpath[0].c_str());
  if(element == NULL) return "";

  for(size_t i = 1; i < bpath.size(); i++)
  {
    element = element->FirstChildElement(bpath[i].c_str());
    if(element == NULL) return "";
  }

  return std::string(element->FirstChild()->ToText()->Value());
}

int conf::getConfigInt(std::string path)
{
  return std::atoi(conf::getConfigString(path).c_str());
}
