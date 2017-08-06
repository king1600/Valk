#pragma once 

#include "collection.hh"

namespace valk {

  class User;
  class Guild;
  class Channel;

  class Color {
  private:
    uint32_t value;
  public:
    uint8_t r, g, b, a;
    Color(const uint8_t _r = 0, const uint8_t _g = 0,
      const uint8_t _b = 0, const uint8_t _a = 0) : 
    r(_r), g(_g), b(_b), a(_a) {}
    Color(const uint32_t val) {
      r = static_cast<uint8_t>((val >> (8 * 0)) & 0xff);
      g = static_cast<uint8_t>((val >> (8 * 0)) & 0xff);
      b = static_cast<uint8_t>((val >> (8 * 0)) & 0xff);
      a = static_cast<uint8_t>((val >> (8 * 0)) & 0xff);
    }
    const uint32_t val() {
      value = 0;
      uint8_t num[4];
      num[0] = r;
      num[1] = g;
      num[2] = b;
      num[3] = a;
      value = *((uint32_t*)(num));
      return value;
    }
  };

  struct ChannelType {
    static const uint8_t GuildText     = 0;
    static const uint8_t UserDM        = 1;
    static const uint8_t GuildVoice    = 2;
    static const uint8_t GroupDM       = 3;
    static const uint8_t GuidlCategory = 4;
  };

  struct MessageType {
    static const uint8_t Default              = 0;
    static const uint8_t RecipientAdd         = 1;
    static const uint8_t RecipientRemove      = 2;
    static const uint8_t Call                 = 3;
    static const uint8_t ChannelNameChange    = 4;
    static const uint8_t ChannelIconChange    = 5;
    static const uint8_t ChannelPinnedMessage = 6;
    static const uint8_t GuildMemberJoin      = 7;
  };

  typedef struct Invite {
    Guild *guild;
    Channel *channel;
    std::string code;
  } Invite;

  typedef struct Overwrite {
    int deny;
    int allow;
    snowflake id;
    std::string type;
  } Overwrite;

  typedef struct Attachment {
    int width;
    int height;
    snowflake id;
    std::string url;
    std::size_t size;
    std::string filename;
    std::string proxy_url;
  } Attachment;

  typedef struct VoiceRegion {
    bool vip;
    bool custom;
    bool optimal;
    std::string id;
    bool deprecated;
    int sample_port;
    std::string name;
    std::string sample_hostname;
  } VoiceRegion;

  typedef struct VoiceState {
    bool deaf;
    bool mute;
    User *user;
    Guild *guild;
    bool suppress;
    bool self_deaf;
    bool self_mute;
    Channel *channel;
    std::string session_id;
  } VoiceState;

  class Role : public Item {
  public:
    bool hoist;
    Color color;
    bool managed;
    Guild *guild;
    std::string name;
    bool mentionable;
    uint32_t position;
    uint32_t permissions;

    inline ~Role() = default;
    inline Role() : Item() {}
    void from(const io::json& data);
    std::string toString() {
      return "<@&" + std::to_string(id) + ">";
    }
  };

  class Emoji : public Item {
  public:
    Guild *guild;
    bool managed;
    std::string name;
    bool require_colons;
    std::vector<Role> roles;

    inline ~Emoji() = default;
    inline Emoji() : Item() {}
    void from(const io::json& data);
    std::string toString() {
      return "<:" + name + ":" + std::to_string(id) + ">";
    }
  };

  typedef struct Reaction {
    bool me;
    Emoji& emoji;
    uint8_t count;
    inline Reaction(Emoji &e) : emoji(e) {}
  } Reaction;
}