#pragma once

#include "misc.hh"
#include "user.hh"
#include "channel.hh"

namespace valk {

  typedef struct Mentions {
    bool everyone;
    std::vector<User*> users;
    std::vector<Role*> roles;
  } Mentions;

  class Message : public Item {
  public:
    User user;
    Member member;
    TextChannel channel;

    bool tts;
    bool pinned;
    uint8_t type;
    snowflake nonce;
    Mentions mentions;
    std::vector<Reaction> reactions;
    std::vector<Attachment> attachments;

    io::Date edited;
    io::Date created;
    std::string content;

    inline Message() : Item() {}
    void from(const io::json &data);
  };

}