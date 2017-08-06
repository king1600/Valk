#pragma once

#include "item.hh"

namespace valk {

  template <typename T>
  class Collection {
  private:
    std::vector<T> items;

  public:
    inline Collection() {}
    inline Collection(const std::vector<T>& i) : 
      items(i) {}
    inline Collection(const std::size_t size) :
      items(size) {}

    T& operator[] (const long index) {
      return items[index < 0 ? items.size() - index : index];
    }

    inline std::vector<T>& get() {
      return items;
    }

    inline const std::size_t size() const {
      return items.size();
    }

    inline typename std::vector<T>::iterator begin() {
      return items.begin();
    }

    inline typename std::vector<T>::const_iterator begin() const {
      return items.begin();
    }

    inline typename std::vector<T>::iterator end() {
      return items.end();
    }

    inline typename std::vector<T>::const_iterator end() const {
      return items.end();
    }


    inline void for_each(const std::function<void(const T&)> apply) const {
      std::for_each(items.begin(), items.end(), apply);
    }

    Collection<T>& operator += (const T& item) {
      items.push_back(item); return *this;
    }

    Collection<T>& operator += (const Collection<T>& list) {
      items.insert(items.end(), list.begin(), list.end()); return *this;
    }

    const bool has(const std::function<bool(const T&)> &check) const {
      return std::find(items.begin(), items.end(), check) != items.end();
    }

    T& find(const std::function<bool(const T&)> check) {
      return *(std::find_if(items.begin(), items.end(), check));
    }

    std::vector<T*> find_all(const std::function<bool(const T&)> check) {
      std::vector<T*> result;
      auto it = std::find_if(items.begin(), items.end(), check);
      while (it != items.end()) {
        result.push_back(&(*it));
        it = std::find_if(++it, items.end(), check);
      }
      return std::move(result);
    }
  };

}