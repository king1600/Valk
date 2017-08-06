#pragma once

#include "uri.hh"

namespace io {

  unsigned char* b64_decode(const char *src, std::size_t len);

  char* b64_encode(const unsigned char *src, std::size_t len);

}