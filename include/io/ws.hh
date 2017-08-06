#pragma once

#include "uri.hh"
#include "ssl.hh"

namespace io {

  class Opcode {
  public:
    static const unsigned char CONT  = 0x00;
    static const unsigned char TEXT  = 0x01;
    static const unsigned char BIN   = 0x02;
    static const unsigned char CLOSE = 0x08;
    static const unsigned char PING  = 0x09;
    static const unsigned char PONG  = 0x0a;
  };

  class Frame {
  public:
    bool fin = true;
    bool masked = true;
    unsigned char rsv1 = 0;
    unsigned char rsv2 = 0;
    unsigned char rsv3 = 0;
    unsigned char opcode = 0;
    std::vector<char> data;
  };

  enum WebsockState { OPEN, CLOSED, CONNECTING };

  class WebsockClient {
  private:
    bool connected;
    Service &service;
    std::shared_ptr<SSLClient> client;

    Uri uri;
    Frame frame;
    WebsockState state;
    std::vector<char> builder;

    void processFrame();

    std::function<void()> on_connect;
    std::function<void(const Frame&)> on_frame;
    std::function<void(const int, const std::string&)> on_close;

  public:
    WebsockClient(Service&);

    const bool isConnected() const;
    void Connect(const std::string& url);
    void Close(int status, const std::string &reason);

    void Send(const std::string &data,
      unsigned char opcode = Opcode::TEXT);

    void onConnect(const std::function<void()>&);
    void onFrame(const std::function<void(const Frame&)>&);
    void onClose(const std::function<void(const int, const std::string&)>&);
  };

}