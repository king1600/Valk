#pragma once

#include "json.hh"
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace io {

  namespace asio = boost::asio;
  namespace ssl  = asio::ssl;
  typedef asio::ip::tcp tcp;
  typedef boost::system::error_code error_code;

  static const error_code Success = 
    boost::system::errc::make_error_code(
      boost::system::errc::success);

  class Timer {
  private:
    asio::deadline_timer timer;
    std::function<void(Timer*)> timer_cb;
  public:
    void *data;

    Timer(asio::io_service& service);
    void cancel();
    void async(const long ms, const std::function<void(Timer*)> &cb);
  };

  class Service {
  private:
    std::shared_ptr<tcp::resolver> resolver;
    std::shared_ptr<ssl::context> ssl_ctx;
    std::shared_ptr<asio::io_service> loop;

  public:
    Service();

    void Run();
    ssl::context& getContext();
    asio::io_service& getService();

    Timer* createTimer();
    Timer* spawn(const long ms, void *data, const std::function<void(Timer*)> &cb);

    void Resolve(const std::string &host, int port,
      std::function<void(const error_code&, tcp::resolver::iterator)> cb);
  };

  class SSLClient {
  private:
    Service &service;
    bool connected = false;
    std::vector<char> builder;
    std::array<char, 8192> buffer;
    std::shared_ptr<ssl::stream<tcp::socket>> sock;

    std::function<void(const error_code&)> on_close;
    std::function<void(const error_code&)> on_connect;
    std::function<void(const std::vector<char>&)> on_read;

    void _read_handler(const error_code&, std::size_t);
    void _connect_handler(const error_code&, tcp::resolver::iterator,
      std::function<void(const error_code&)> callback);
    void _connect(const tcp::endpoint&, tcp::resolver::iterator&,
      std::function<void(const error_code&)> callback);

  public:
    SSLClient(Service& loop);

    void Connect(const std::string& host, int port);
    void Send(const char* data, const std::size_t len);
    void Close(const error_code& err, bool callback = true);

    const bool isConnected() const;
    void onClose(std::function<void(const error_code&)>);
    void onConnect(std::function<void(const error_code&)>);
    void onRead(std::function<void(const std::vector<char>&)>);
  };
}