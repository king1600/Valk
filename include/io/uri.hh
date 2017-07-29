#pragma once

#include "ssl.hh"

namespace io {

  class Uri {
  public:
    int port;
    bool ssl;
    std::string url;
    std::string host;
    std::string path;
    std::string proto;
    std::string query;
    std::map<std::string, std::string> params;

    Uri() = default;
    void Parse(const std::string &_url);
    Uri(const std::string &_url) { Parse(_url); }
  };

}