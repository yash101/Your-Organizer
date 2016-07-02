#ifndef DEFAULTCONFIGURATION_H
#define DEFAULTCONFIGURATION_H
#include "tinyxml2.h"
#include <string>

#ifndef CONFIGURATION_FILE_LOCATION
#define CONFIGURATION_FILE_LOCATION "configuration.xml"
#endif

namespace conf
{
  extern tinyxml2::XMLDocument config;
  void init();
  std::string getConfigString(std::string path);
  int getConfigInt(std::string path);
}

#endif // DEFAULTCONFIGURATION_H
