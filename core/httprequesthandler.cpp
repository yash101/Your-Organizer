#include "httprequesthandler.h"
#include "../base/string_algorithms.h"

#include <stdio.h>
#include <regex>
using namespace yo;

void yo::httpRequestHandler(http::HttpSession& session)
{
  for(std::multimap<size_t, HostHandler>::iterator it = hostnames.begin(); it != hostnames.end(); ++it)
  {
    if(it->second.matching_method & mm_compare)
    {
      if(session.getPath() == it->second.matcher)
      {
        if(it->second(session)) return;
      }
    }
    else if(it->second.matching_method & mm_regex)
    {
      if(std::regex_match(session.getPath(), std::regex(it->second.matcher)))
      {
        if(it->second(session)) return;
      }
    }
  }

  session.response.data = "ERROR: Could not resolve handler for your request!";
  session.response.type = http::DataSource::String;
}

bool HostHandler::operator()(http::HttpSession& session)
{
  for(std::multimap<size_t, HttpAction>::iterator it = actions.begin(); it != actions.end(); ++it)
  {
    if(it->second.matching_method & mm_compare)
    {
      if(session.getPath() == it->second.matcher)
        if(it->second(session)) return true;
    }
    else if(it->second.matching_method & mm_regex)
    {
      if(std::regex_match(session.getPath(), std::regex(it->second.matcher)))
        if(it->second(session)) return true;
    }
  }

  return false;
}

bool HttpAction::operator()(http::HttpSession& session)
{
  return function_pointer(session, data);
}



//Returns a static file
bool yo::static_handler(http::HttpSession& session, void* data)
{
  std::string path = session.getPath();

  if(path.front() == '/') path.erase(path.begin());
  if(path.size() == 0 || path.size() == 1) return false;

  //Resource sanitization
  base::replaceAll(path, "\\", "/");
  base::replaceAll(path, "./", "");
  base::replaceAll(path, "../", "");
  base::replaceAll(path, "~/", "");


  if(data != NULL)
  {
    session.response.data = ((const char*) data) + path;
  }
  else
    session.response.data = path;

  session.response.type = http::DataSource::File;
  fprintf(stdout, "Proc: [%s]\n", session.response.data.c_str());

  return true;
}


std::multimap<size_t, HostHandler> yo::hostnames;
HttpAction::HttpAction() :
  matching_method(0),
  data(NULL)
{}
HostHandler::HostHandler() :
  matching_method(0)
{}
