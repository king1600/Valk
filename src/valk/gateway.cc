#include "gateway.hh"
#include "client.hh"
#include "utils.hh"
#include <iostream>

static const unsigned char DISPATCH              = 0;
static const unsigned char HEARTBEAT             = 1;
static const unsigned char IDENTIFY              = 2;
static const unsigned char STATUS_UPDATE         = 3;
static const unsigned char VOICE_STATE_UPDATE    = 4;
static const unsigned char VOICE_SERVER_PING     = 5;
static const unsigned char RESUME                = 6;
static const unsigned char RECONNECT             = 7;
static const unsigned char REQUEST_GUILD_MEMBERS = 8;
static const unsigned char INVALID_SESSION       = 9;
static const unsigned char HELLO                 = 10;
static const unsigned char HEARTBEAT_ACK         = 11;

static inline const std::string OSName() {
  #ifdef _WIN32
    return "win32";
  #elif _WIN64
    return "win64";
  #elif __linux__
    return "linux";
  #elif __APPLE__ || __MACH__
    return "darwin";
  #elif __unix || __unix__
    return "unix";
  #elif __FreeBSD__
    return "freebsd";
  #else
    return "other";
  #endif
}

valk::Gateway::Gateway
(valk::Client *client, const std::size_t id, const std::size_t max)
  : shard_id(id), max_shards(max), beat_acked(true), seq(0)
{
  this->resume = false;
  this->client = client;
  conn = std::make_shared<io::WebsockClient>(client->service);
}

void valk::Gateway::Send(const unsigned char op, const io::json& data) {
  io::json to_send = {{"op", op}, {"d", data}};
  std::cout << "Sending: " << to_send.dump(2) << std::endl;
  if (conn.get() != nullptr) conn->Send(to_send.dump());
}

void valk::Gateway::start_beating() {
  heartbeat = client->service.createTimer();
  beat();
}

void valk::Gateway::stop_beating() {
  if (heartbeat != nullptr) {
    delete heartbeat;
    heartbeat = nullptr;
  }
  seq = 0;
}

void valk::Gateway::beat() {
  if (!conn->isConnected()) {
    stop_beating();
    return;
  }
  if (!beat_acked) {
    resume = true;
    conn->Close(1011, "Heartbeat stopped");
    stop_beating();
    return;
  }

  io::json data;
  if (seq > 0) data = seq;
  else data = io::json::parse("null");
  Send(HEARTBEAT, data);
  beat_acked = false;
  heartbeat->async(interval, [this](io::Timer *t) {
    beat();
  });
}

void valk::Gateway::identify() {
  io::json data;
  if (!resume)
    data = {
      {"token", client->token},
      {"properties", {
        {"$os", OSName()},
        {"$browser", valk::LIBNAME},
        {"$device", valk::LIBNAME}
      }},
      {"compress", false},
      {"large_threshold", 250},
      {"shard", io::json::array({shard_id, max_shards})},
      {"presence", {
        {"game", nullptr},
        {"status", "online"},
        {"since", nullptr},
        {"afk", false}
      }}
    };
  else
    data = {
      {"token", client->token},
      {"session_id", session_id},
      {"seq", seq}
    };

  Send(resume ? RESUME : IDENTIFY, data);
}

void valk::Gateway::Connect(const std::string &_url) {
  if (url.empty()) 
    url = _url +  (_url.back() != '/' ? "/" : "" )
      + "?v=" + valk::GATEWAY_VERSION + "&encoding=json";
  std::cout << "[valk] Connecting to: " << url << std::endl;

  conn->onFrame([this](const io::Frame &frame) {
    std::string data_str(frame.data.begin(), frame.data.end());
    if (data_str.front() != '{' || data_str.back() != '}') return;
    io::json data = io::json::parse(data_str);
    if (data.find("s") != data.end() && !data["s"].is_null()) seq = data["s"];

    std::cout << "Got: " << data.dump(2) << std::endl;

    switch (data["op"].get<unsigned char>()) {
      case HELLO: {
        interval = data["d"]["heartbeat_interval"];
        identify();
        break;
      }
      case DISPATCH: {
        dispatch(data["t"], data["d"]);
        break;
      }
      case HEARTBEAT_ACK: {
        std::cout << "[valk] HEARTBEAT ACK" << std::endl;
        beat_acked = true;
        break;
      }
      case RECONNECT: {
        resume = true;
        conn->Close(1001, "");
        break;
      }
      case REQUEST_GUILD_MEMBERS: {
        break;
      }
      case VOICE_STATE_UPDATE: {
        break;
      }
      case INVALID_SESSION: {
        resume = data["d"].get<bool>();
        client->service.spawn(4000, nullptr, [this](io::Timer *timer) {
          conn->Close(1011, "");
          delete timer;
        });
        break;
      }
    }
  });

  conn->onClose([this](const int code, const std::string &reason) {
    std::cout << "[valk] Client closed: " << code << " " << reason << std::endl;
    std::cout << "[valk] Reconnecting..." << std::endl;
    stop_beating();
    client->service.spawn(50, nullptr, [this](io::Timer *timer) {
      Connect(url);
      delete timer;
    });
  });

  conn->Connect(url);
}

void valk::Gateway::dispatch(const std::string &event, const io::json &data) {
  std::cout << "Handling event: " << event << std::endl;

  if (event == "READY") {
    session_id = data["session_id"];
    client->user.from(data["user"]);
    client->users += client->user;
    for (const io::json &_guild : data["guilds"]) {
      valk::Guild guild;
      guild.from(_guild);
      client->guilds += std::move(guild);
    }
  } else if (event == "RESUMED") {

  } else if (event == "CHANNEL_CREATE") {
    
  } else if (event == "CHANNEL_UPDATE") {
    
  } else if (event == "CHANNEL_DELETE") {
    
  } else if (event == "CHANNEL_PINS_UPDATE") {
    
  } else if (event == "GUILD_CREATE") {
    valk::snowflake id = data["id"];
    std::cout << "Id: " << data["id"] << std::endl;
    valk::Guild &guild = client->guilds.find(
      [&id](const valk::Guild &g) { return g.id == id; });
    guild.from(data);
    std::size_t loaded = 0;
    for (const valk::Guild &g : client->guilds)
      if (!g.unavailable)
        loaded++;
    if (loaded == client->guilds.size()) {
      std::cout << "starting heartbeat" << std::endl;
      start_beating();
    }
    
  } else if (event == "GUILD_UPDATE") {
    
  } else if (event == "GUILD_DELETE") {
    
  } else if (event == "GUILD_BAN_ADD") {
    
  } else if (event == "GUILD_BAN_REMOVE") {
    
  } else if (event == "GUILD_EMOJIS_UPDATE") {
    
  } else if (event == "GUILD_INTEGRATIONS_UPDATE") {
    
  } else if (event == "GUILD_MEMBER_ADD") {
    
  } else if (event == "GUILD_MEMBER_REMOVE") {
    
  } else if (event == "GUILD_MEMBER_UPDATE") {
    
  } else if (event == "GUILD_MEMBERS_CHUNK") {
    
  } else if (event == "GUILD_ROLE_CREATE") {
    
  } else if (event == "GUILD_ROLE_UPDATE") {
    
  } else if (event == "GUILD_ROLE_DELETE") {
    
  } else if (event == "MESSAGE_CREATE") {
    
  } else if (event == "MESSAGE_UPDATE") {
    
  } else if (event == "MESSAGE_DELETE") {
    
  } else if (event == "MESSAGE_DELETE_BULK") {
    
  } else if (event == "MESSAGE_REACTION_ADD") {
    
  } else if (event == "MESSAGE_REACTION_REMOVE") {
    
  } else if (event == "MESSAGE_REACTION_REMOVE_ALL") {
    
  } else if (event == "PRESENCE_UPDATE") {
    
  } else if (event == "USER_UPDATE") {
    
  } else if (event == "VOICE_STATE_UPDATE") {
    
  } else if (event == "VOICE_SERVER_UPDATE") {
    
  } else if (event == "WEBHOOKS_UPDATE") {
    
  }
}