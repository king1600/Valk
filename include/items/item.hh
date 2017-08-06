#pragma once

#include "io/json.hh"
#include "io/date.hh"

namespace io {
  using json = nlohmann::json;
}

namespace valk {

  using snowflake = uint64_t;

  class Item {
  public:
    snowflake id;
    inline Item() = default;
    inline Item(snowflake _id) : id(_id) {}

    virtual std::string toString() = 0;
    virtual void from(const io::json& data) = 0;

    inline const bool operator== (const Item& other) {
      return this->id == other.id;
    }
  };
}