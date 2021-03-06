#pragma once

#include <string>

namespace valk {

  static const std::string LIBNAME = "Valk";
  static const std::string HTTP_VERSION = "7";
  static const std::string GATEWAY_VERSION = "6";
  static const std::string BASE_HOST = "discordapp.com";
  static const std::string BASE_ENDPOINT = "/api/v" + HTTP_VERSION;

  static const std::string VERSION_MAJOR = "0";
  static const std::string VERSION_MINOR = "1";
  static const std::string VERSION_PATCH = "0";
  static const std::string VERSION_STRING = 
    VERSION_MAJOR + "." + VERSION_MINOR + "." + VERSION_PATCH;
}