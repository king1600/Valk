#pragma once

#include "io/ws.hh"
#include "items/items.hh"

namespace valk {

  class Client;
  class Gateway {
  private:
    std::string url;
    std::size_t shard_id;
    std::size_t max_shards;
    std::shared_ptr<io::WebsockClient> conn;

    bool resume;
    long interval;
    bool beat_acked;
    std::size_t seq;
    io::Timer *heartbeat;
    std::string session_id;

    void beat();
    void identify();
    void stop_beating();
    void start_beating();
    void dispatch(const std::string &event, const io::json &data);

  public:
    Client *client;

    
    Gateway(Client*, const std::size_t, const std::size_t);

    void Send(const unsigned char op, const io::json &data);

    void Connect(const std::string &url);
  };

}