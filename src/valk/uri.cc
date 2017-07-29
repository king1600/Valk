#include "io/uri.hh"

void io::Uri::Parse(const std::string &_url) {
  url = _url;
  std::string buf;
  std::size_t pos;
  
  pos = url.find("://");
  proto = url.substr(0, pos);
  buf = url.substr(pos + 3);
  pos = buf.find("/");
  host = buf.substr(0, pos);
  query = buf.substr(pos);
  pos = query.find("?");
  pos = pos == std::string::npos ? query.size() : pos;
  path = query.substr(0, pos);

  std::transform(host.begin(), host.end(), host.begin(), ::tolower);
  std::transform(proto.begin(), proto.end(), proto.begin(), ::tolower);
  ssl = proto == "https" || proto == "wss";

  pos = host.find(":");
  if (pos != std::string::npos) {
    port = std::stoi(host.substr(pos + 1), nullptr, 10);
  } else port = ssl ? 443 : 80;

  pos = query.find('?');
  if (!query.empty() && pos != std::string::npos) {
    std::stringstream qstream(query.substr(pos + 1));
    while (std::getline(qstream, buf, '&')) {
      pos = buf.find('=');
      if (pos != std::string::npos)
        params[buf.substr(0, pos)] = buf.substr(pos + 1);
    }
  }
}