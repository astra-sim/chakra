#ifndef CHAKRA_FEEDER_V3_ET_FEEDER_H
#define CHAKRA_FEEDER_V3_ET_FEEDER_H

#include <fstream>
#include "cache.h"
#include "dependancy_solver.h"
#include "et_def.pb.h"

namespace Chakra {
namespace FeederV3 {
using NodeId = uint64_t;
using ETFeederId = uint64_t;
using ChakraNode = ChakraProtoMsg::Node;

class ETFeederNode;

class ETFeeder {
 public:
  ChakraProtoMsg::GlobalMetadata global_metadata;
  ETFeeder(const std::string& file_path)
      : chakra_file(file_path, std::ios::binary | std::ios::in | std::ios::app),
        _operator_id(_operator_id_cnt++),
        dependancy_resolver(USE_DATA_DEPS, USE_CTRL_DEPS) {
    if (!chakra_file.is_open())
      throw std::runtime_error("Failed to open file " + file_path);
    this->build_index_cache();
  }

  ~ETFeeder() {
    // no explict cache release. Will be kicked-out natually.
    index_map.clear();
    chakra_file.close();
  }

  DependancyResolver& getDependancyResolver() {
    return dependancy_resolver;
  }

  std::shared_ptr<ETFeederNode> lookupNode(uint64_t node_id);
  bool hasNodesToIssue();
  std::shared_ptr<ETFeederNode> getNextIssuableNode();
  void pushBackIssuableNode(uint64_t node_id);
  void freeChildrenNodes(uint64_t node_id);

  // legacy interface
  void addNode(std::shared_ptr<ETFeederNode> node);
  void removeNode(uint64_t node_id);

 private:
  constexpr size_t DEFAULT_CACHE_SIZE = 16384;
  constexpr bool USE_DATA_DEPS = true;
  constexpr bool USE_CTRL_DEPS = true;

  static uint64_t _operator_id_cnt;
  uint64_t _operator_id;

  // shared global cache for storing chakra msgs.
  static Cache<std::tuple<ETFileOperatorId, NodeId>, ChakraNode> _node_cache;

  std::ifstream chakra_file;
  std::unordered_map<NodeId, std::streampos> index_map;
  DependancyResolver dependancy_resolver;

  void build_index_cache();
  const std::shared_ptr<const ChakraNode> get_raw_chakra_node(NodeId node_id);
  friend class ETFeederNode;
};
} // namespace FeederV3
} // namespace Chakra

#endif
