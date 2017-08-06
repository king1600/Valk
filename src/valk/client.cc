#include "client.hh"

valk::Client::Client() {
  api = std::make_shared<io::RestClient>(service);
}

void valk::Client::login(const std::string token) {
  api->SetToken(token);
  this->token = token;

  api->get("/gateway/bot", {}, [this](const io::json &resp) {
    const std::size_t shard_count = resp["shards"];
    const std::string url = resp["url"];
    for (std::size_t i = 0; i < shard_count; i++) {
      valk::Gateway gateway(this, i, shard_count);
      shards.push_back(std::move(gateway));
    }
    for (valk::Gateway& gateway : shards)
      gateway.Connect(url);
  });

  service.Run();
}