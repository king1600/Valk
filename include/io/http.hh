#pragma once

#include "uri.hh"
#include "json.hh"
#include <deque>

namespace io {

  class Response {
  public:
    int status;
    std::string body;
    std::string reason;
    std::map<std::string, std::string> headers;
  };

  using json = nlohmann::json;
  using HeaderCallback = std::function<void(std::string&, std::string &)>;
  using HttpCallback = std::function<void(const std::string&, const Response&)>;

  class HttpCallbackQuery {
  public:
    std::string route;
    HttpCallback callback;
  };

  enum ParseState {
    METHOD, HEADERS, BODY, END
  };

  class HttpParser {
  private:
    Response resp;
    int csize = -1;
    bool on_chunk = true;
    bool chunked = false;
    std::size_t chunk_size;
    HeaderCallback on_header;
    std::deque<HttpCallbackQuery> callbacks;
    ParseState state = ParseState::METHOD;

  public:
    HttpParser();

    static std::string gzip(const std::string &);
    static std::string gunzip(const std::string &);

    void onHeader(const HeaderCallback& cb);
    void Feed(const std::vector<char>& data);
    void AddCallback(const std::string &route, const HttpCallback &callback);
  };

}