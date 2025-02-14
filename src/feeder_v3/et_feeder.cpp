#include "et_feeder.h"
#include "protobuf_util.h"

using namespace Chakra::FeederV3;

void ETFeeder::addNode(std::shared_ptr<ETFeederNode> node) {
  throw std::runtime_error(
      "For offloaded ETFeeder, the graph is static and readonly, and addNode is not supported");
}

void ETFeeder::removeNode(uint64_t node_id) {
  static bool firstTime = true;
  if (firstTime) {
    firstTime = false;
    std::cerr
        << "For offloaded ETFeeder, the graph is static and readonly, and removeNode is ignored"
        << std::endl;
  }
}

bool ETFeeder::hasNodesToIssue() {
  return !this->dependancy_resolver.get_dependancy_free_nodes().empty();
}

std::shared_ptr<ETFeederNode> ETFeeder::getNextIssuableNode() {
  const auto& node_id =
      *(this->dependancy_resolver.get_dependancy_free_nodes().begin());
  this->dependancy_resolver.take_node(node_id);
  return this->lookupNode(node_id);
}

void ETFeeder::pushBackIssuableNode(uint64_t node_id) {
  this->dependancy_resolver.push_back_node(node_id);
}

std::shared_ptr<ETFeederNode> ETFeeder::lookupNode(uint64_t node_id) {}

void ETFeeder::freeChildrenNodes(uint64_t node_id) {
  this->dependancy_resolver.finish_node(node_id);
}

uint64_t ETFeeder::_operator_id_cnt = 0;
Cache<std::tuple<ETFeeder, NodeId>, ChakraNode> ETFeeder::_node_cache(
    ETFeeder::DEFAULT_CACHE_SIZE);

void ETFeeder::build_index_cache() {
  this->chakra_file.clear();
  this->chakra_file.seekg(0, std::ios::beg);
  ChakraNode node;
  bool ret =
      ProtobufUtils::readMessage(this->chakra_file, this->global_metadata);
  if (!ret)
    throw std::runtime_error("Failed to read global metadata");
  std::streampos last_pos = this->chakra_file.tellg();
  while (true) {
    ret = ProtobufUtils::readMessage(this->chakra_file, node);
    if (!ret)
      break;
    const auto& node_id = node.id();
    this->index_map[node_id] = last_pos;
    last_pos = this->chakra_file.tellg();
  }
  this->chakra_file.clear();
  this->chakra_file.seekg(0, std::ios::beg);
}

const std::shared_ptr<const ChakraNode> ETFeeder::get_raw_chakra_node(
    NodeId node_id) {
  auto key = std::make_tuple(this->_operator_id, node_id);
  const auto node = ETFeeder::_node_cache.get_or_null_locked(key);
  if (node) {
    // hit
    return node;
  }

  // miss
  if (this->index_map.find(node_id) == this->index_map.end())
    throw std::runtime_error(
        "Node " + std::to_string(node_id) + " not found in index");
  const auto& pos = this->index_map[node_id];
  this->chakra_file.seekg(pos);
  ChakraNode node_msg;
  ProtobufUtils::readMessage(this->chakra_file, node_msg);
  ETFeeder::_node_cache.put(key, node_msg);
  return ETFeeder::_node_cache.get_locked(key);
}
