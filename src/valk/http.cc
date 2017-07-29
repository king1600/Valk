#include "io/http.hh"
#include <iostream>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>

static std::vector<std::string> Split(std::string str, const std::string &delim) {
  std::size_t pos;
  std::string token;
  std::vector<std::string> results;
  while ((pos = str.find(delim)) != std::string::npos) {
    token = str.substr(0, pos);
    results.push_back(token);
    str.erase(0, pos + delim.length());
  }
  if (!str.empty()) results.push_back(str);
  return results;
}

io::HttpParser::HttpParser() {
  on_header = [](const std::string& k, const std::string &v){};
}

void io::HttpParser::onHeader(const HeaderCallback &callback) {
  on_header = callback;
}

void io::HttpParser::AddCallback
(const std::string &route, const io::HttpCallback &callback)
{
  io::HttpCallbackQuery query;
  query.route = std::move(route);
  query.callback = std::move(callback);
  callbacks.push_front(std::move(query));
}

std::string io::HttpParser::gzip(const std::string &data) {
  namespace bio = boost::iostreams;
  std::stringstream compressed;
  std::stringstream origin(data);
  bio::filtering_streambuf<bio::input> output;
  output.push(bio::gzip_compressor(bio::gzip_params(
    bio::gzip::best_compression)));
  output.push(origin);
  bio::copy(output, compressed);
  return compressed.str();
}

std::string io::HttpParser::gunzip(const std::string &data) {
  namespace bio = boost::iostreams;
  std::stringstream compressed(data);
  std::stringstream decompressed;
  bio::filtering_streambuf<bio::input> output;
  output.push(bio::gzip_decompressor());
  output.push(compressed);
  bio::copy(output, decompressed);
  return decompressed.str();
}

void io::HttpParser::Feed(const std::vector<char>& raw) {
  std::string data(raw.begin(), raw.end());

  std::vector<std::string> lines = Split(
    std::string(data.begin(), data.end()), "\r\n");

  for (std::string &line : lines) {

    if (state == io::ParseState::METHOD) {
      line = line.substr(line.find(" ") + 1);
      const std::size_t space = line.find(" ");
      resp.reason = line.substr(space + 1);
      resp.status = std::stoi(line.substr(0, space), nullptr, 10);
      state = io::ParseState::HEADERS;

    } else if (state == io::ParseState::HEADERS) {
      if (line.empty()) { state = ParseState::BODY; continue; }
      const std::size_t space = line.find(": ");
      std::string key = line.substr(0, space);
      std::string value = line.substr(space + 2);
      resp.headers[key] = value;
      if (key.find("Transfer-Encoding") != std::string::npos) {
        if (value.find("chunked") != std::string::npos) {
          chunked = true; on_chunk = true;
        }
      } else if (key == "Content-Length") {
        csize = std::stoi(value, nullptr, 10);
      }
      on_header(key, value);

    } else if (state == ParseState::BODY) {
      if (csize > 0) {
        resp.body += line;
        if (resp.body.size() >= (unsigned int)csize)
          state = io::ParseState::END;
      } else if (chunked) {
        if (line.size() < 1) continue;
        if (on_chunk) {
          on_chunk = false;
          chunk_size = std::stoi(line, nullptr, 16);
          if (chunk_size == 0) state = io::ParseState::END;
        } else {
          on_chunk = true;
          resp.body += line;
        }
      }

    } else break;
  }

  if (state == io::ParseState::END) {
    state = io::ParseState::METHOD;
    csize = -1;
    chunked = true;

    if (resp.headers.find("Content-Encoding") != resp.headers.end())
      if (resp.headers["Content-Encoding"].find("gzip") != std::string::npos)
        resp.body = gunzip(resp.body);

    if (!callbacks.empty()) {
      io::HttpCallbackQuery query = std::move(callbacks.back());
      callbacks.pop_back();
      std::cout << "Got callback" << std::endl;
      query.callback(query.route, resp);
      resp.body.clear();
      resp.headers.clear();
    }
  }
}