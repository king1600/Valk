#pragma once

#include "item.hh"

namespace valk {

  class Channel : public Item {
  public:
    inline Channel() : Item() {}

    std::string toString() {
      return "<#" + std::to_string(id) + ">";
    }
  };

  class TextChannel : public Channel {
  public:
    void from(const io::json &data);
    inline TextChannel() : Channel() {}
  };

  class VoiceChannel : public Channel {
  public:
    void from(const io::json &data);
    inline VoiceChannel() : Channel() {}
  };
}