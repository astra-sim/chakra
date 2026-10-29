// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 Advanced Micro Devices, Inc.

#pragma once

#include <json/json.hpp>
#include <functional>
#include <set>
#include <string>

using json = nlohmann::json;

class JSONNode {
 public:
  /**
   * !!!IMPORTANT!!! This list needs to be consistent with the NodeType list in
   * extern/graph_frontend/chakra/schema/protobuf/et_def.proto
   */
  enum NodeType : int {
    INVALID_NODE = 0,
    METADATA_NODE = 1,
    MEM_LOAD_NODE = 2,
    MEM_STORE_NODE = 3,
    COMP_NODE = 4,
    COMM_SEND_NODE = 5,
    COMM_RECV_NODE = 6,
    COMM_COLL_NODE = 7
  };

  /**
   * List of all CollectiveCommType
   * !!!IMPORTANT!!! This list needs to be consistent with the CommCollNodeType
   * list in extern/graph_frontend/chakra/schema/protobuf/et_def.proto
   */
  enum CollectiveCommType : int {
    ALL_REDUCE = 0,
    REDUCE = 1,
    ALL_GATHER = 2,
    GATHER = 3,
    SCATTER = 4,
    BROADCAST = 5,
    ALL_TO_ALL = 6,
    REDUCE_SCATTER = 7,
    REDUCE_SCATTER_BLOCK = 8,
    BARRIER = 9,
    CUSTOM_MSCCL = 10,
    INVALID_COMM = 99
  };

  JSONNode();
  JSONNode(const JSONNode& t);
  JSONNode(json data, uint64_t id);

  /** Getters */
  uint64_t id() const;
  std::string name() const;
  NodeType type() const;
  CollectiveCommType getCommType() const;
  bool isCPUOp() const;
  // Always in picoseconds; the constructor converts the legacy "runtime"
  // (microseconds) key to match "runtime(ps)".
  uint64_t getRuntime() const;
  uint64_t getNumOps() const;
  uint64_t getTensorSize() const;
  uint32_t getCommPriority() const;
  uint64_t getCommSize() const;
  uint32_t getCommSrc() const;
  uint32_t getCommDst() const;
  uint32_t getCommTag() const;
  const std::string& getCustomCommPlanFile() const noexcept;

  /** Setters */
  void setID(uint64_t new_node_id) noexcept;
  void setName(const std::string& new_node_name) noexcept;
  void setType(int new_node_type) noexcept;
  void setType(NodeType new_node_type) noexcept;
  void setCommType(CollectiveCommType new_comm_type) noexcept;
  void setIsCPUOp(bool new_is_cpu_op) noexcept;
  void setRuntime(uint64_t new_runtime) noexcept;
  void setNumOps(uint64_t new_num_ops) noexcept;
  void setTensorSize(uint64_t new_tensor_size) noexcept;
  void setCommPriority(uint32_t new_comm_priority) noexcept;
  void setCommSize(uint64_t new_comm_size) noexcept;
  void setCommSrc(uint32_t new_comm_src) noexcept;
  void setCommDst(uint32_t new_comm_dst) noexcept;
  void setCommTag(uint32_t new_comm_tag) noexcept;
  void setCustomCommPlanFile(const std::string& filename) noexcept;

  /** Dependency graph utils */
  void addDepUnresolvedParentID(uint64_t node_id);
  std::set<uint64_t>& getDepUnresolvedParentIDs();
  void addChild(const JSONNode& node);
  void addDataDep(const JSONNode& node);
  std::set<uint64_t>& getChildren();
  std::set<uint64_t>& getDataDeps();

  // Define the == operator for comparison
  bool operator==(const JSONNode& other) const;

  // Overload the assignment operator
  JSONNode& operator=(const JSONNode& other);

  // Return a string name of the collective communication type
  static std::string getCollectiveCommTypeStr(CollectiveCommType comm_type);

 private:
  uint64_t node_id = 0;
  std::string node_name = "";
  NodeType node_type = NodeType::INVALID_NODE;
  CollectiveCommType comm_type = INVALID_COMM;
  bool is_cpu_op = false;
  uint64_t runtime = 0;
  uint64_t num_ops = 0;
  uint64_t tensor_size = 0;
  uint32_t comm_priority = 0;
  uint64_t comm_size = 0;
  uint32_t comm_tag = 0;
  uint32_t comm_src = 0;
  uint32_t comm_dst = 0;

  // Collective communication plan file for custom collective (applicable to
  // comm_type == CUSTOM_MSCCL)
  std::string custom_comm_plan_file;

  /** Dynamic sets of node IDs for constructing dependency graph */
  std::set<uint64_t> dep_unresolved_parent_ids;
  std::set<uint64_t> data_dep_ids; // parent node IDs
  std::set<uint64_t> child_node_ids;
};

// Define a custom hash function for unordered set
namespace std {
template <>
struct hash<JSONNode> {
  std::size_t operator()(const JSONNode& node) const {
    std::size_t h1 = std::hash<int64_t>()(node.id());
    std::size_t h2 = std::hash<std::string>()(node.name());
    std::size_t h3 = std::hash<int>()(node.type());
    std::size_t h4 = std::hash<bool>()(node.isCPUOp());
    std::size_t h5 = std::hash<int64_t>()(node.getRuntime());

    // A prime number for bit manipulation
    const std::size_t prime = 31;

    // Combine the hash of the current member with the hashes of the previous
    // members
    std::size_t hash = h1;
    hash = hash * prime + h2;
    hash = hash * prime + h3;
    hash = hash * prime + h4;
    hash = hash * prime + h5;

    return hash;
  }
};
} // namespace std

// Compare function for JSON node for priority queue
struct CompareJSONNodesGT {
  template <class T>
  constexpr bool operator()(const T lhs, const T rhs) const {
    return lhs.id() > rhs.id();
  }
};
