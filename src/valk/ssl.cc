#include "io/ssl.hh"
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

void io::Service::Run() {
  loop->run();
}

io::ssl::context& io::Service::getContext() {
  return *(ssl_ctx.get());
}

io::asio::io_service& io::Service::getService() {
  return *(loop.get());
}

io::Service::Service() {
  loop = std::make_shared<io::asio::io_service>();
  resolver = std::make_shared<io::tcp::resolver>(getService());
  ssl_ctx = std::make_shared<io::ssl::context>(io::ssl::context::sslv23);
}

void io::Service::Resolve(const std::string &host, int port,
  std::function<void(const io::error_code&, io::tcp::resolver::iterator)> cb)
{
  io::tcp::resolver::query query(host.c_str(), std::to_string(port).c_str());
  resolver->async_resolve(query, cb);
}

io::Timer* io::Service::createTimer() {
  io::Timer *timer = new io::Timer(getService());
  return timer;
}

io::Timer* io::Service::spawn
(const long delay, void *data, const std::function<void(Timer*)> &callback)
{
  io::Timer *timer = new io::Timer(getService());
  timer->data = data;
  timer->async(delay, std::move(callback));
  return timer;
}

////////////////////////////////////////////////////////////////////////

io::Timer::Timer(io::asio::io_service &service) : timer(service), data(nullptr) {
}

void io::Timer::cancel() {
  timer.cancel();
}

void io::Timer::async(const long ms, const std::function<void(Timer*)> &callback) {
  timer_cb = std::move(callback);
  timer.expires_from_now(boost::posix_time::millisec(ms));
  timer.async_wait([this](const io::error_code& err){
    if (!err) timer_cb(this);
  });
}

////////////////////////////////////////////////////////////////////////

io::SSLClient::SSLClient(io::Service& loop) : service(loop) {
  sock = std::make_shared<io::ssl::stream<io::tcp::socket>>(
    service.getService(), service.getContext());
  sock->set_verify_mode(io::ssl::verify_none);

  on_close = [](const io::error_code& err){};
  on_connect = [](const io::error_code& err){};
  on_read = [](const std::vector<char>& data){};
}

const bool io::SSLClient::isConnected() const {
  return connected;
}

void io::SSLClient::onClose(std::function<void(const io::error_code&)> cb) {
  on_close = cb;
}

void io::SSLClient::onConnect(std::function<void(const io::error_code&)> cb) {
  on_connect = cb;
}

void io::SSLClient::onRead(std::function<void(const std::vector<char>&)> cb) {
  on_read = cb;
}

void io::SSLClient::Send(const char* data, const std::size_t len) {
  if (!connected) return;
  io::asio::async_write(*(sock.get()), io::asio::buffer(data, len),
    [this](const io::error_code& e, std::size_t written) { if (e) Close(e); });
}

void io::SSLClient::_connect(const io::tcp::endpoint& endpoint,
  io::tcp::resolver::iterator& it, std::function<void(const io::error_code&)> callback)
{
  sock->lowest_layer().async_connect(endpoint, boost::bind(
    &io::SSLClient::_connect_handler, this,
    io::asio::placeholders::error, ++it, callback));
}

void io::SSLClient::Close(const io::error_code& err, bool callback) {
  if (!connected) return;
  io::error_code ec;
  connected = false;
  sock->lowest_layer().cancel(ec);
  sock->async_shutdown([&](const io::error_code &error) {
    io::error_code e;
    sock->lowest_layer().close(e);
    if (callback)
      on_close(e ? e : (err ? err : ec));
  });
}

void io::SSLClient::_connect_handler(const io::error_code& err,
  io::tcp::resolver::iterator it, std::function<void(const io::error_code&)> callback)
{
  if (!err) {
    callback(io::Success);
  } else if (it != io::tcp::resolver::iterator()) {
    Close(io::Success, false);
    _connect(*it, ++it, callback);
  } else {
    callback(err);
  }
}

void io::SSLClient::_read_handler(const io::error_code& err, std::size_t size) {
  if (err) {
    Close(err);
  } else {
    builder.insert(builder.end(), buffer.data(), buffer.data() + size);
    if (size < buffer.max_size()) {
      on_read(builder);
      builder.clear();
    }
    sock->async_read_some(io::asio::buffer(buffer),
      boost::bind(&io::SSLClient::_read_handler, this,
        io::asio::placeholders::error,
        io::asio::placeholders::bytes_transferred));
  }
}

void io::SSLClient::Connect(const std::string& host, int port) {
  if (connected) return;
  service.Resolve(host, port,
  [&](const io::error_code& err, io::tcp::resolver::iterator it) {
    if (err) {
      on_connect(err);
      return;
    }
    _connect(*it, it, [this](const io::error_code &err) {
      sock->async_handshake(io::ssl::stream_base::client,
      [this](const io::error_code &err) {
        connected = true;
        on_connect(err);
        sock->async_read_some(io::asio::buffer(buffer),
          boost::bind(&io::SSLClient::_read_handler, this,
            io::asio::placeholders::error,
            io::asio::placeholders::bytes_transferred));
      });
    });
  });
}