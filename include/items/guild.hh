#pragma once

#include "misc.hh"
#include "user.hh"
#include "channel.hh"

namespace valk {

  class Guild : public Item {
  public:
    Member owner;
    std::string name;
    std::string icon;
    std::string splash;
    std::string region;
    
    bool large;
    io::Date joined;
    bool unavailable;

    int mfa_level;
    int afk_timeout;
    int verify_level;
    int member_count;
    int default_notify;
    int explicit_filter;
    VoiceChannel afk_channel;

    std::vector<Role> roles;
    std::vector<Emoji> emojis;
    std::vector<Member> members;
    std::vector<Channel*> channels;
    std::vector<VoiceState> voice_states;

    inline ~Guild() = default;
    inline Guild() : Item() {}
    void from(const io::json &data);

    std::string toString() {
      return name;
    }
  };

}