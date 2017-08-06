#include "io/rest.hh"
#include "utils.hh"
#include <ctime>
#include <iostream>

io::RestClient::RestClient(io::Service &loop) : service(loop) {
  client = nullptr;
  _connect();
  parser.onHeader([this](std::string &key, std::string &value) {
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    if (key.find("set-cookie") != std::string::npos) 
      cookies.push_back(value.substr(0, value.find(";")));
  });
}

void io::RestClient::SetToken(const std::string &token) {
  this->token = token;
}

void io::RestClient::pushRequest(const std::string &data) {
  if (client.get() != nullptr && client->isConnected())
    client->Send(data.c_str(), data.size());
  else
    writes.push_front(std::move(data));
}

io::RestRoute::RestRoute() : limited(false) {
}

const bool io::RestRoute::isLimited() const {
  return limited;
}

const bool io::RestRoute::hasPending() const {
  return !queued.empty();
}

void io::RestRoute::setLimited(bool state) {
  limited = state;
}

void io::RestRoute::getPending(io::RestRequest &req) {
  req = std::move(queued.back());
  queued.pop_back();
}

void io::RestRoute::addPending(const io::RestRequest &req) {
  queued.push_front(std::move(req));
}

const std::size_t io::RestRoute::hasLeft() const {
  return queued.size();
}

void io::RestClient::get(const std::string& endpoint,
  const io::json &data, const io::RestCallback &callback) {
  Request("GET", endpoint, data, callback);
}

void io::RestClient::post(const std::string& endpoint,
  const io::json &data, const io::RestCallback &callback) {
  Request("POST", endpoint, data, callback);
}

void io::RestClient::del(const std::string& endpoint,
  const io::json &data, const io::RestCallback &callback) {
  Request("DELETE", endpoint, data, callback);
}

void io::RestClient::_connect() {
  if (client == nullptr)
    client = std::make_shared<io::SSLClient>(service);
  else
    client.reset(new io::SSLClient(service));

  client->onRead([this](const std::vector<char> &data) {
    parser.Feed(data);
  });

  client->onConnect([&](const io::error_code &err) {
    if (err) {
      std::cerr << "[Valk] Failed to connect: " << err.message() << std::endl;
      return;
    }
    if (!writes.empty()) {
      std::string data;
      while (!writes.empty()) {
        data = std::move(writes.back());
        writes.pop_back();
        pushRequest(data);
      }
    }
  });

  client->onClose([&](const io::error_code &err) {
    std::cout << "Client closed: " << err.message() << std::endl;
    std::cout << "Respawning..." << std::endl;
    service.getService().post([this]() {
      _connect();
    });
  });

  client->Connect(valk::BASE_HOST, 443);
}

void io::RestClient::Request(const std::string& method,
  const std::string &endpoint, const io::json &data, const io::RestCallback &callback)
{
  std::ostringstream request;

  std::size_t rend = endpoint.substr(1).find("/");
  rend = rend == std::string::npos ? endpoint.size() : rend + 1;
  std::string route_str = method + ";" + endpoint.substr(0, rend);
  if (routes.find(route_str) == routes.end()) {
    io::RestRoute route;
    routes.insert(std::make_pair(route_str, std::move(route)));
  }

  request << method << " " << valk::BASE_ENDPOINT << endpoint << " HTTP/1.1\r\n"
      << "Host: " << valk::BASE_HOST << ":443\r\n"
      << "Authorization: Bot " << token << "\r\n"
      << "User-Agent: DisocrdBot (" << valk::LIBNAME
        << ", " << valk::VERSION_STRING << ")\r\n"
      << "Accept: */*\r\n" 
      << "Accept-Encoding: gzip\r\n"
      << "Connection: keep-alive\r\n";
  if (cookies.size() > 0) {
    request << "Cookie: ";
    for (std::size_t i = 0; i < cookies.size(); i++)
      request << cookies[i] << (i == cookies.size() - 1 ? "" : "; ");
    request << "\r\n";
  }
  std::string json_data = data.dump();
  if (json_data.size() > 4) {
    std::cout << "Encoding: " << json_data << std::endl;
    json_data = parser.gzip(json_data);
    request << "Content-Encoding: gzip\r\n"
        << "Content-Length: " << json_data.size() << "\r\n"
        << "Content-Type: application/json; charset=UTF-8\r\n"
        << json_data;
  } else request << "\r\n";

  io::RestRequest req;
  req.data = std::move(data);
  req.method = std::move(method);
  req.endpoint = std::move(endpoint);
  req.callback = std::move(callback);

  if (globalRoute.isLimited()) {
    globalRoute.addPending(std::move(req));
    return;
  }
  routes[route_str].addPending(std::move(req));
  bool limited = routes[route_str].isLimited();
  if (limited) return;

  parser.AddCallback(route_str, [this](const std::string &route, const io::Response &resp) {
    io::json data = io::json::parse(resp.body);
    io::RestRequest rreq;
    routes[route].getPending(rreq);

    if (resp.headers.find("X-RateLimit-Remaining") != resp.headers.end()) {
      if (std::stoi(resp.headers.at("X-RateLimit-Remaining"), nullptr, 10) < 1) {
        long wait_time;
        if (resp.headers.find("X-RateLimit-Reset") != resp.headers.end()) {
          const long now = static_cast<long>(std::time(nullptr));
          long reset = std::stol(resp.headers.at("X-RateLimit-Reset"), nullptr, 10);
          wait_time = (reset - now) * 1000;
        } else if (resp.headers.find("Retry-After") != resp.headers.end()) {
          wait_time = std::stol(resp.headers.at("Retry-After"), nullptr, 10);
        }

        if (data.find("global") != data.end()) {
          std::string global = data["global"];
          if (global == "true") {
            globalRoute.setLimited(true);
            service.spawn(wait_time, nullptr, [this](io::Timer *task) {
              io::RestRequest request;
              globalRoute.setLimited(false);
              const std::size_t remaining = globalRoute.hasLeft();
              for (std::size_t i = 0; i < remaining; i++) {
                globalRoute.getPending(request);
                Request(request.method, request.endpoint,
                  request.data, request.callback);
              }
            });
          }
        }

        routes[route].setLimited(true);
        service.spawn(wait_time, new std::string(route), [this](io::Timer *task) {
          std::string *route_str = static_cast<std::string*>(task->data);
          io::RestRequest request;

          this->routes[*route_str].setLimited(false);
          const std::size_t remaining = this->routes[*route_str].hasLeft();
          for (std::size_t i = 0; i < remaining; i++) {
            this->routes[*route_str].getPending(request);
            this->Request(request.method, request.endpoint,
              request.data, request.callback);
          }
          
          delete route_str;
          delete task;
        });
      }
    }
    rreq.callback(data);
  });
  
  pushRequest(request.str());
}