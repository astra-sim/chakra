// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 Advanced Micro Devices, Inc.

#pragma once

#include <feeder/json_node.h>
#include <feeder_v3/et_feeder.h>
#include <list>
#include <queue>

using json = nlohmann::json;

enum format { Protobuf, JSON };

/// WrapperNode class wraps protobuf and JSON
class WrapperNode {
 private:
  enum format format_type_;
  Chakra::FeederV3::ETFeeder* et_feeder_;
  std::shared_ptr<Chakra::FeederV3::ETFeederNode> node_{nullptr};
  std::ifstream jsonfile_;
  json data_;
  bool json_et_complete_;
  int num_json_nodes;

  // Base directory of the currently open protobuf trace file, used to
  // resolve a relative custom_comm_plan_file the same way v1's
  // ETFeeder::readNode() did.
  std::string protobuf_trace_base_dir_;

  /** Data structures for protobuf */
  std::queue<std::shared_ptr<Chakra::FeederV3::ETFeederNode>>
      push_back_queue_proto;

  /** Data structures for JSON nodes */

  // ID of the current JSON node
  int64_t curr_json_node_id;
  // Map of node IDs to JSON nodes
  std::unordered_map<uint64_t, JSONNode> all_json_node_map;
  // Set of dependence-free JSON nodes
  std::set<uint64_t> dep_free_json_node_ids;
  // Queue of nodes waiting for HW resources
  std::queue<uint64_t> push_back_queue_json_node_ids;

  // Chakra node id -> path to custom collective (MSCCL++) plan file, from an
  // external plan map.
  std::unordered_map<uint64_t, std::string> custom_comm_plan_map_;

  // Rewrite each node's custom_comm_plan_file from a path relative to the
  // workload file (or absolute) into a path resolvable from the process CWD.
  void resolvePlanPaths(const std::string& workload_filename);

 public:
  WrapperNode();
  WrapperNode(const WrapperNode& t);
  WrapperNode(std::string filename);
  ~WrapperNode();
  void releaseMemory();
  void createWrapper(std::string filename);
  void createSingleCustomCommNode(
      const std::string& filename,
      uint64_t comm_size);
  void removeNode(uint64_t node_id);
  void readAllNodes();
  void resolveDep();
  void pushBackIssuableNode(uint64_t node_id);
  void freeChildrenNodes(uint64_t node_id);
  bool isValidNode();
  void push_to_queue();
  bool is_queue_empty();
  void queue_front();
  void pop_from_queue();
  void getNextIssuableNode();
  uint64_t getNodeID();
  std::string getNodeName();
  int getNodeType();
  bool isCPUOp();
  // Node runtime/replay duration, always in picoseconds regardless of the
  // trace format's native unit (protobuf and legacy JSON store microseconds).
  uint64_t getRuntime();
  uint64_t getNumOps();
  uint64_t getTensorSize();
  int64_t getCommType();
  uint32_t getCommPriority();
  uint64_t getCommSize();
  uint32_t getCommSrc();
  uint32_t getCommDst();
  uint32_t getCommTag();
  std::string getCustomCommPlanFile();
  /**
   * Install a chakra node id -> plan file map, consulted by
   * getCustomCommPlanFile() ahead of a node's own attribute.
   *
   * @param plan_map node id -> path to the MSCCL++ plan file
   */
  void setCustomCommPlanMap(std::unordered_map<uint64_t, std::string> plan_map);
  bool hasNodesToIssue();
  void lookupNode(uint64_t node_id);
  JSONNode* getCurrentJSONNodePtr();
};
