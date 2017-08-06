#pragma once

#include "io/rest.hh"
#include "gateway.hh"
#include "items/collection.hh"

namespace valk {

  class Client {
  private:
    std::vector<Gateway> shards;

  public:
    std::string token;
    io::Service service;
    std::shared_ptr<io::RestClient> api;

    User user;
    Collection<User> users;
    Collection<Guild> guilds;
    Collection<Channel*> channel;

    Client();

    void login(const std::string token);
  };

}