#ifndef CHAKRA_FEEDER_V3_CACHE_H
#define CHAKRA_FEEDER_V3_CACHE_H

#include <list>
#include <memory>
#include <shared_mutex>
#include <unordered_map>

namespace Chakra {
namespace FeederV3 {

template <typename K, typename V>
class Cache {
 public:
  Cache(size_t capacity) : capacity(capacity) {}
  void put(const K& key, const V& value);
  bool has(const K& key) const;
  const std::weak_ptr<const T> get(const K& key) const;
  const std::shared_ptr<const T> get_locked(const K& key) const;
  const std::weak_ptr<const T> get_or_null(const K& key) const;
  const std::shared_ptr<const T> get_or_null_locked(const K& key) const;

  void remove(const K& key);

  ~Cache() {
    this->cache.clear();
    this->lru.clear();
  }

 private:
  size_t capacity;
  std::unordered_map<
      K,
      std::pair<std::shared_ptr<const V>, typename std::list<K>::iterator>>
      cache;
  std::list<K> lru;
  std::shared_mutex cache_mutex;
};

} // namespace FeederV3
} // namespace Chakra

#endif
