#include "valk.hh"
#include <iostream>

int main() {
  try {
    valk::Client client;

    

    const std::string token = "";
    client.login(token);

  } catch (std::exception &ex) {
    std::cerr << "Exception: " << ex.what() << std::endl;
  }
}