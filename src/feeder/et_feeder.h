// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 Advanced Micro Devices, Inc.

#pragma once

#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <feeder/et_feeder_node.h>
#include <third_party/utils/protoio.hh>

namespace Chakra {
struct CompareNodes {
  template <class T>
  constexpr bool operator()(
      const std::shared_ptr<T> lhs,
      const std::shared_ptr<T> rhs) const {
    return lhs->getChakraNode()->id() > rhs->getChakraNode()->id();
  }
};

class ETFeeder {
 public:
  ETFeeder(std::string filename);
  ~ETFeeder();

  void addNode(std::shared_ptr<ETFeederNode> node);
  void removeNode(uint64_t node_id);
  bool hasNodesToIssue();
  std::shared_ptr<ETFeederNode> getNextIssuableNode();
  void pushBackIssuableNode(uint64_t node_id);
  std::shared_ptr<ETFeederNode> lookupNode(uint64_t node_id);
  void freeChildrenNodes(uint64_t node_id);
  void readGlobalMetadata();
  std::shared_ptr<ETFeederNode> readNode();
  void readNextWindow();
  void resolveDep();

 private:
  ProtoInputStream trace_;
  const uint32_t window_size_;
  bool et_complete_;
  // Directory of the trace file, used to resolve an inline
  // custom_comm_plan_file given relative to it.
  std::string plan_base_dir_;

  std::unordered_map<uint64_t, std::shared_ptr<ETFeederNode>> dep_graph_{};
  std::unordered_set<uint64_t> dep_free_node_id_set_{};
  std::priority_queue<
      std::shared_ptr<ETFeederNode>,
      std::vector<std::shared_ptr<ETFeederNode>>,
      CompareNodes>
      dep_free_node_queue_{};
  std::unordered_set<std::shared_ptr<ETFeederNode>> dep_unresolved_node_set_{};
};

} // namespace Chakra
