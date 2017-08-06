#pragma once

#include "item.hh"

namespace valk {

  class User : public Item {  
  public:
    bool bot;
    bool verified;
    bool mfa_enabled;
    std::string email;
    std::string avatar;
    std::string discrim;
    std::string username;

    inline ~User() = default;
    inline User() : Item() {}
    void from(const io::json& data);

    std::string toString() {
      return "<@" + std::to_string(id) + ">";
    }
  };

  class Member : public Item {
  protected:
    User *_user;
    void from(const io::json& data);
  public:
    bool deaf;
    bool mute;
    io::Date joined;
    std::string nick;
    std::string name;

    inline Member() : Item() {}
    inline Member(const io::json& data) : Item(data) {}

    std::string toString() {
      return "<@!" + std::to_string(id) + ">";
    }
  };
}