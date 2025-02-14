#include "etfeeder_v3/cache.h"

using namespace Chakra::FeederV3;

template <typename K, typename V>
void Cache<K, V>::put(const K& key, const V& value) {
  std::unique_lock lock(cache_mutex);
  if (this->cache.find(key) != this->cache.end()) {
    // hit and update
    this->lru.erase(this->cache[key].second);
    this->lru.push_back(key);
    this->cache[key].second = --this->lru.end();
    this->cache[key].first = std::make_shared<const V>(value);
  } else {
    // miss
    while (this->cache.size() >= this->capacity) {
      // evict
      auto victim = this->lru.front();
      this->cache.erase(victim);
      this->lru.pop_front();
    }
    // and put new
    this->lru.push_back(key);
    this->cache[key] =
        std::make_pair(std::make_shared<const V>(value), --this->lru.end());
  }
}

template <typename K, typename V>
bool Cache<K, V>::has(const K& key) const {
  std::shared_lock lock(cache_mutex);
  return this->cache.find(key) != this->cache.end();
}

template <typename K, typename V>
const std::weak_ptr<const V> Cache<K, V>::get(const K& key) const {
  std::shared_lock lock(cache_mutex);
  if (this->cache.find(key) == this->cache.end()) {
    throw std::runtime_error("Key not found in cache");
  }
  const std::weak_ptr<const V> value(this->cache.at(key).first);
  return value;
}

template <typename K, typename V>
const std::shared_ptr<const V> Cache<K, V>::get_locked(const K& key) const {
  std::shared_lock lock(cache_mutex);
  if (this->cache.find(key) == this->cache.end()) {
    throw std::runtime_error("Key not found in cache");
  }
  return this->cache.at(key).first;
}

template <typename K, typename V>
const std::weak_ptr<const V> Cache<K, V>::get_or_null(const K& key) const {
  std::shared_lock lock(cache_mutex);
  if (this->cache.find(key) == this->cache.end()) {
    return std::weak_ptr<const V>();
  }
  const std::weak_ptr<const V> value(this->cache.at(key).first);
  return value;
}

template <typename K, typename V>
const std::shared_ptr<const V> Cache<K, V>::get_or_null_locked(
    const K& key) const {
  std::shared_lock lock(cache_mutex);
  if (this->cache.find(key) == this->cache.end()) {
    return std::shared_ptr<const V>();
  }
  return this->cache.at(key).first;
}

template <typename K, typename V>
void Cache<K, V>::remove(const K& key) {
  std::unique_lock lock(cache_mutex);
  if (this->cache.find(key) == this->cache.end()) {
    throw std::runtime_error("Key not found in cache");
  }
  this->lru.erase(this->cache[key].second);
  this->cache.erase(key);
}
