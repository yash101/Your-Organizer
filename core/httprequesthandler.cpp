#include "httprequesthandler.h"
#include <regex>
using namespace yo;

void yo::httpRequestHandler(http::HttpSession& session)
{
  for(std::multimap<size_t, HostHandler>::iterator it = hostnames.begin(); it != hostnames.end(); ++it)
  {
    if(it->second.matching_method & mm_match)
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
    if(it->second.matching_method & mm_match)
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
  session.response.data = "Hello World!";
  session.response.type = http::DataSource::String;
  return true;
}

std::multimap<size_t, HostHandler> yo::hostnames;
HttpAction::HttpAction() :
  matching_method(0)
{}
HostHandler::HostHandler() :
  matching_method(0)
{}
