#include "valk.hh"
#include <iostream>

int main() {
  try {
    io::Service loop;

    io::RestClient client(loop);
    client.SetToken("");

    client.Request("GET", "/gateway/bot", {}, [&](const io::json &data) {
      std::cout << "Response received: " << data.dump(4) << std::endl;
      client.Request("GET", "/gateway/bot", {}, [&](const io::json &ddata) {
        std::cout << "Second response received: " << ddata.dump(4) << std::endl;
        client.Request("GET", "/gateway/bot", {}, [](const io::json &dddata) {
          std::cout << "Last received: " << dddata.dump(4) << std::endl;
        });
      });
    });
    
    loop.Run();
  } catch (std::exception &ex) {
    std::cerr << "Exception: " << ex.what() << std::endl;
  }
}