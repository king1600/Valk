#include "io/ws.hh"
#include "io/b64.hh"
#include <iostream>
#include <random>

static std::random_device Rng;
static std::mt19937 Random(Rng());
static std::uniform_int_distribution<int> Rgen(1, 255);
static const char *HANDSHAKE = 
"GET %s HTTP/1.1\r\n"
"Host: %s:%d\r\n"
"Upgrade: WebSocket\r\n"
"Connection: Upgrade\r\n"
"Sec-WebSocket-Key: %s\r\n"
"Sec-WebSocket-Version: 13\r\n"
"\r\n";

static inline unsigned char randByte() {
  return (unsigned char)(Rgen(Random));
}

static inline std::size_t GetFrameSize(const io::Frame &frame) {
  std::size_t i = 2 + (frame.masked ? 4 : 0) + frame.data.size();
  if (frame.data.size() >= 0x7e && frame.data.size() < 0xffff) i += 2;
  else if (frame.data.size() >= 0x10000) i += 8;
  return i;
}

static const std::size_t FrameUnpack(io::Frame &frame, const char *data, std::size_t len) {
  int i, count = 0;
  std::size_t size = 0;
  std::size_t offset = 0;
  frame.data.clear();

  frame.fin  = (data[offset] & 0x80) != 0 ? 1 : 0;
  frame.rsv1 = (data[offset] & 0x40) != 0 ? 1 : 0;
  frame.rsv2 = (data[offset] & 0x20) != 0 ? 1 : 0;
  frame.rsv3 = (data[offset] & 0x10) != 0 ? 1 : 0;
  frame.opcode = data[offset++] & 0x0f;
  
  frame.masked = (data[offset] & 0x80) != 0 ? 1 :0;
  size = static_cast<std::size_t>(data[offset++] & (~0x80));
  if (size == 0x7f) count = 8;
  else if (size == 0x7e) count = 2;
  if (count > 0) size = 0;
  while (count-- > 0)
    size |= (data[offset++] & 0xff) << (8 * count);
  frame.data.reserve(size);
  
  char mask[4];
  if (frame.masked)
    for (i = 0; i < 4; i++)
      mask[i] = data[offset++];

  for (i = 0; i < (signed)size; i++)
    frame.data.push_back(frame.masked ? 
     (data[offset + i] ^ mask[i % 4]) : data[offset + i]);
  return offset + size;
}

static void FramePack(io::Frame &frame, char *data) {
  int i;
  std::size_t offset = 0;

  data[offset] = (char)(frame.fin ? 0x80 : 0);
  data[offset] |= (frame.rsv1 ? 0x40 : 0);
  data[offset] |= (frame.rsv2 ? 0x20 : 0);
  data[offset] |= (frame.rsv3 ? 0x10 : 0);
  data[offset++] |= (char)(frame.opcode);
    
  char masked = (char)(frame.masked ? 0x80 : 0);
  const std::size_t size = frame.data.size();
  if (size <= 0x7d) {
    data[offset++] = (char)(masked | size);
  } else if (size >= 0x7e && size < 65536) {
    data[offset++] = (char)(masked | 0x7e);
    for (i = 8; i >= 0; i -= 8)
      data[offset++] = (char)((size >> i) & 0xff);
  } else {
    data[offset++] = (char)(masked | 0x7f);
    for (i = 56; i >= 0; i -= 8)
      data[offset++] = (char)((size >> i) & 0xff);
  }
  
  unsigned char *mask = nullptr;
  if (frame.masked) {
    mask = new unsigned char[4];
    for (i = 0; i < 4; i++) {
      mask[i] = randByte();
      data[offset++] = mask[i];
    }
  }
  
  for (i = 0; i < (signed)size; i++)
    data[offset + i] = (char)(!frame.masked ?
      frame.data[i] : (frame.data[i] ^ mask[i % 4]));
  if (mask != nullptr) delete mask;
}

io::WebsockClient::WebsockClient(io::Service &service) : service(service) {
  connected = false;
  client = std::make_shared<io::SSLClient>(service);
}

const bool io::WebsockClient::isConnected() const {
  return connected;
}

void io::WebsockClient::onConnect(const std::function<void()>& cb) {
  on_connect = cb;
}

void io::WebsockClient::onFrame(const std::function<void(const io::Frame&)>& cb) {
  on_frame = cb;
}

void io::WebsockClient::onClose(const std::function<void(const int, const std::string&)>& cb) {
  on_close = cb;
}

void io::WebsockClient::Send(const std::string &data, unsigned char opcode) {
  if (connected) {
    io::Frame frame;
    frame.fin = true;
    frame.masked = true;
    frame.opcode = opcode;
    frame.data.insert(frame.data.begin(), data.begin(), data.end());
    
    const std::size_t frame_size = GetFrameSize(frame);
    char buffer[frame_size] = { 0 };
    FramePack(frame, buffer);
    if (client.get() != nullptr && state == io::WebsockState::OPEN)
      client->Send(buffer, frame_size);
  }
}

void io::WebsockClient::Close(int status, const std::string &reason) {
  if (connected) {
    io::Frame frame;
    frame.fin = true;
    frame.masked = false;
    frame.opcode = io::Opcode::CLOSE;
    frame.data.reserve(2 + reason.size());
    frame.data.push_back(static_cast<char>((status >> 8) & 0xff));
    frame.data.push_back(static_cast<char>((status >> 0) & 0xff));
    frame.data.insert(frame.data.end(), reason.begin(), reason.end());
    
    connected = false;
    state = io::WebsockState::CLOSED;
    const std::size_t frame_size = GetFrameSize(frame);
    char buffer[frame_size] = { 0 };
    FramePack(frame, buffer);
    if (client.get() != nullptr)
      client->Send(buffer, frame_size);
  }
}

void io::WebsockClient::processFrame(const io::Frame &frame) {
  if (!frame.fin || frame.opcode == io::Opcode::CONT) {
    builder.insert(builder.end(), frame.data.begin(), frame.data.end());
    return;
  } else {
    if (builder.size() > 0) {
      builder.insert(builder.end(), frame.data.begin(), frame.data.end());
      frame.data = builder;
      builder.clear();
    }
    if (frame.opcode == io::Opcode::CLOSE) {
      int code = ((frame.data[0] >> 8) & 0xff) | ((frame.data[1] >> 8) & 0xff);
      std::string reason(frame.data.begin() + 2, frame.data.end());
      if (state == io::WebsockState::OPEN)
        Close(code, reason);
      client->Close(io::Success);
      on_close(code, reason);
    } else if (frame.opcode == io::Opcode::PING) {
      Send(std::string(frame.data.begin(), frame.data.end()), io::Opcode::PONG);
    } else if (frame.opcode == io::Opcode::PONG) {

    } else on_frame(frame);
  }
}

void io::WebsockClient::Connect(const std::string &url) {
  uri.Parse(url);

  client->onRead([this](const std::vector<char> &data) {
    if (!connected) {
      std::string header(data.begin(), data.end());
      if (header.find("HTTP/1.1 101") == 0) {
        connected = true;
        state = io::WebsockState::OPEN;
      } else {
        on_close(1005, "");
        client->Close(io::Success);
      }

    } else {
      std::cout << "Received: ";
      std::cout.write(&data[0], data.size());
      std::cout << std::endl;

      std::size_t unpacked = FrameUnpack(frame, &data[0], data.size());
      processFrame(frame);
      while (unpacked < data.size()) {
        unpacked = FrameUnpack(frame, &data[0], data.size());
      }
    }
  });

  client->onConnect([this](const io::error_code &err) {
    if (err) {
      state = io::WebsockState::CLOSED;
      on_close(1005, "Failed to connect");
      return;
    }

    char http_data[1024] = { 0 };
    unsigned char key[17] = { 0 };
    for (unsigned char i = 0; i < 16; i++)
      key[i] = randByte();
    std::sprintf(http_data, HANDSHAKE,
      uri.query.c_str(), uri.host.c_str(), uri.port,
      static_cast<const char*>(b64_encode(key, 16)));
    client->Send(http_data, std::strlen(http_data));
    state = io::WebsockState::CONNECTING;
  });

  client->Connect(uri.host, uri.port);
}