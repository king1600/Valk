#pragma once

#include "http.hh"

namespace io {

  using RestCallback = std::function<void(const json&)>;

  static const json JSON_EMPTY = json::parse("{}");
  static const RestCallback CB_NONE = [](const json& j){};

  class RestRequest {
  public:
    json data;
    std::string method;
    std::string endpoint;
    RestCallback callback;
  };

  class RestRoute {
  private:
    bool limited;
    std::deque<RestRequest> queued;
  public:
    RestRoute();

    void setLimited(bool state);    
    const bool isLimited() const;
    const bool hasPending() const;
    void getPending(RestRequest &req);
    const std::size_t hasLeft() const;
    void addPending(const RestRequest &req);
  };

  class RestClient {
  private:
    Service& service;
    std::string token;
    HttpParser parser;
    std::deque<std::string> writes;
    std::vector<std::string> cookies;
    std::shared_ptr<SSLClient> client;
    std::map<std::string, RestRoute> routes;

    void _connect();

    void pushRequest(const std::string &data);

  public:
    RestClient(Service &loop);

    void SetToken(const std::string &token);
    void Request(const std::string& method, const std::string &endpoint,
      const json &data = JSON_EMPTY, const RestCallback &cb = CB_NONE);
  };

}