// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 Advanced Micro Devices, Inc.

#include <feeder/json_node.h>
#include <iostream>

// JSONNode default constructor
JSONNode::JSONNode() {}

// JSONNode copy constructor
JSONNode::JSONNode(const JSONNode& t) {
  node_id = t.node_id;
  node_name = t.node_name;
  node_type = t.node_type;
  comm_type = t.comm_type;
  is_cpu_op = t.is_cpu_op;
  runtime = t.runtime;
  num_ops = t.num_ops;
  tensor_size = t.tensor_size;
  comm_priority = t.comm_priority;
  comm_size = t.comm_size;
  comm_tag = t.comm_tag;
  comm_src = t.comm_src;
  comm_dst = t.comm_dst;
  dep_unresolved_parent_ids = t.dep_unresolved_parent_ids;
  data_dep_ids = t.data_dep_ids;
  child_node_ids = t.child_node_ids;
  custom_comm_plan_file = t.custom_comm_plan_file;
}

// JSONNode constructor
JSONNode::JSONNode(json data, uint64_t id) {
  try {
    node_id = data["workload_graph"][id]["Id"];
  } catch (...) {
    std::cerr << "node_id not specified in ET" << std::endl;
  }
  try {
    node_name = data["workload_graph"][id]["Name"];
  } catch (...) {
    std::cerr << "node_name not specified in ET" << std::endl;
  }
  try {
    node_type = data["workload_graph"][id]["NodeType"];
  } catch (...) {
    std::cerr << "node_type not specified in ET" << std::endl;
  }
  try {
    is_cpu_op = data["workload_graph"][id]["is_cpu_op"];
  } catch (...) {
    std::cerr << "is_cpu_op not specified in ET" << std::endl;
  }
  // getRuntime() always returns picoseconds. A node carries either
  // "runtime(ps)" or the legacy "runtime" (microseconds); specifying both
  // is an error.
  const auto& node_json = data["workload_graph"][id];
  const auto has_ps_runtime = node_json.contains("runtime(ps)");
  const auto has_us_runtime = node_json.contains("runtime");
  if (has_ps_runtime && has_us_runtime) {
    std::cerr << "Node " << id
              << " specifies both runtime(ps) and runtime; specify exactly one"
              << std::endl;
    exit(-1);
  }
  if (has_ps_runtime) {
    runtime = node_json["runtime(ps)"].get<uint64_t>();
  } else if (has_us_runtime) {
    runtime = node_json["runtime"].get<uint64_t>() * 1000000ull;
  }
  try {
    auto data_deps =
        data["workload_graph"][id]["data_deps"].get<std::vector<uint64_t>>();
    for (auto node_id : data_deps) {
      data_dep_ids.insert(node_id);
    }
  } catch (...) {
    std::cerr << "data deps not specified in ET" << std::endl;
  }
  /* 	COMM_SEND_NODE = 5,
          COMM_RECV_NODE = 6,
          COMM_COLL_NODE = 7 	*/
  if (node_type == 5 || node_type == 6 || node_type == 7) {
    try {
      tensor_size = data["workload_graph"][id]["tensor_size"];
    } catch (...) {
    }
    try {
      comm_type = data["workload_graph"][id]["comm_type"];
    } catch (...) {
    }
    try {
      comm_priority = data["workload_graph"][id]["comm_priority"];
    } catch (...) {
      comm_priority = 0; // Protobuf defaults to 0
    }
    try {
      comm_size = data["workload_graph"][id]["comm_size"];
    } catch (...) {
    }
    try {
      comm_src = data["workload_graph"][id]["comm_src"];
    } catch (...) {
    }
    try {
      comm_dst = data["workload_graph"][id]["comm_dst"];
    } catch (...) {
    }
    try {
      comm_tag = data["workload_graph"][id]["comm_tag"];
    } catch (...) {
    }
    try {
      custom_comm_plan_file =
          data["workload_graph"][id]["custom_comm_plan_file"];
    } catch (...) {
    }
  }
}

// Node id
uint64_t JSONNode::id() const {
  return node_id;
}

// Node name
std::string JSONNode::name() const {
  return node_name;
}

// Node type
JSONNode::NodeType JSONNode::type() const {
  return node_type;
}

// Comm type
JSONNode::CollectiveCommType JSONNode::getCommType() const {
  return comm_type;
}

// Check if CPU OP
bool JSONNode::isCPUOp() const {
  return is_cpu_op;
}

// Runtime
uint64_t JSONNode::getRuntime() const {
  return runtime;
}

// Num ops
uint64_t JSONNode::getNumOps() const {
  return num_ops;
}

// Tensor size
uint64_t JSONNode::getTensorSize() const {
  return tensor_size;
}

// Comm priority
uint32_t JSONNode::getCommPriority() const {
  return comm_priority;
}

// Comm size
uint64_t JSONNode::getCommSize() const {
  return comm_size;
}

// Comm src
uint32_t JSONNode::getCommSrc() const {
  return comm_src;
}

// Comm dst
uint32_t JSONNode::getCommDst() const {
  return comm_dst;
}

// Comm tag
uint32_t JSONNode::getCommTag() const {
  return comm_tag;
}

// Custom comm plan file
const std::string& JSONNode::getCustomCommPlanFile() const noexcept {
  return custom_comm_plan_file;
}

// setters
void JSONNode::setID(const uint64_t new_node_id) noexcept {
  node_id = new_node_id;
}

void JSONNode::setName(const std::string& new_node_name) noexcept {
  node_name = new_node_name;
}

void JSONNode::setType(int new_node_type) noexcept {
  node_type = static_cast<NodeType>(new_node_type);
}

void JSONNode::setType(NodeType new_node_type) noexcept {
  node_type = new_node_type;
}

void JSONNode::setCommType(
    JSONNode::CollectiveCommType new_comm_type) noexcept {
  comm_type = new_comm_type;
}

void JSONNode::setIsCPUOp(const bool new_is_cpu_op) noexcept {
  is_cpu_op = new_is_cpu_op;
}

void JSONNode::setRuntime(const uint64_t new_runtime) noexcept {
  runtime = new_runtime;
}

void JSONNode::setNumOps(const uint64_t new_num_ops) noexcept {
  num_ops = new_num_ops;
}

void JSONNode::setTensorSize(const uint64_t new_tensor_size) noexcept {
  tensor_size = new_tensor_size;
}

void JSONNode::setCommPriority(const uint32_t new_comm_priority) noexcept {
  comm_priority = new_comm_priority;
}

void JSONNode::setCommSize(const uint64_t new_comm_size) noexcept {
  comm_size = new_comm_size;
}

void JSONNode::setCommSrc(const uint32_t new_comm_src) noexcept {
  comm_src = new_comm_src;
}

void JSONNode::setCommDst(const uint32_t new_comm_dst) noexcept {
  comm_dst = new_comm_dst;
}

void JSONNode::setCommTag(const uint32_t new_comm_tag) noexcept {
  comm_tag = new_comm_tag;
}

void JSONNode::setCustomCommPlanFile(const std::string& filename) noexcept {
  custom_comm_plan_file = filename;
}

/**
 * Dependency graph util functions
 */

// Dependency unresolved parent IDs
void JSONNode::addDepUnresolvedParentID(uint64_t node_id) {
  dep_unresolved_parent_ids.insert(node_id);
}

// Get dependency unresolved parent IDs
std::set<uint64_t>& JSONNode::getDepUnresolvedParentIDs() {
  return dep_unresolved_parent_ids;
}

// Add child
void JSONNode::addChild(const JSONNode& node) {
  child_node_ids.insert(node.id());
}

// Add data_deps
void JSONNode::addDataDep(const JSONNode& node) {
  data_dep_ids.insert(node.id());
}

// Get children
std::set<uint64_t>& JSONNode::getChildren() {
  return child_node_ids;
}

// Get parent IDs
std::set<uint64_t>& JSONNode::getDataDeps() {
  return data_dep_ids;
}

/** == operator */
bool JSONNode::operator==(const JSONNode& other) const {
  return node_id == other.node_id && node_name == other.node_name &&
      node_type == other.node_type && comm_type == other.comm_type &&
      is_cpu_op == other.is_cpu_op && runtime == other.runtime &&
      num_ops == other.num_ops && tensor_size == other.tensor_size &&
      comm_priority == other.comm_priority && comm_size == other.comm_size &&
      comm_tag == other.comm_tag && comm_src == other.comm_src &&
      comm_dst == other.comm_dst &&
      dep_unresolved_parent_ids == other.dep_unresolved_parent_ids &&
      data_dep_ids == other.data_dep_ids &&
      child_node_ids == other.child_node_ids &&
      custom_comm_plan_file == other.custom_comm_plan_file;
}

/** Assignment operator */
JSONNode& JSONNode::operator=(const JSONNode& other) {
  if (this != &other) {
    // Copy all member variables
    node_id = other.node_id;
    node_name = other.node_name;
    node_type = other.node_type;
    comm_type = other.comm_type;
    is_cpu_op = other.is_cpu_op;
    runtime = other.runtime;
    num_ops = other.num_ops;
    tensor_size = other.tensor_size;
    comm_priority = other.comm_priority;
    comm_size = other.comm_size;
    comm_tag = other.comm_tag;
    comm_src = other.comm_src;
    comm_dst = other.comm_dst;
    dep_unresolved_parent_ids = other.dep_unresolved_parent_ids;
    data_dep_ids = other.data_dep_ids;
    child_node_ids = other.child_node_ids;
    custom_comm_plan_file = other.custom_comm_plan_file;
  }
  return *this;
}

std::string JSONNode::getCollectiveCommTypeStr(
    JSONNode::CollectiveCommType comm_type) {
  switch (comm_type) {
    case ALL_REDUCE:
      return "ALL_REDUCE";
    case REDUCE:
      return "REDUCE";
    case ALL_GATHER:
      return "ALL_GATHER";
    case GATHER:
      return "GATHER";
    case SCATTER:
      return "SCATTER";
    case BROADCAST:
      return "BROADCAST";
    case ALL_TO_ALL:
      return "ALL_TO_ALL";
    case REDUCE_SCATTER:
      return "REDUCE_SCATTER";
    case REDUCE_SCATTER_BLOCK:
      return "REDUCE_SCATTER_BLOCK";
    case BARRIER:
      return "BARRIER";
    case CUSTOM_MSCCL:
      return "CUSTOM_MSCCL";
    case INVALID_COMM:
      return "INVALID_COMM";
    default: {
      std::cerr << "Unsupported collective comm_type=" << comm_type
                << std::endl;
      exit(1);
    }
  }
  return "";
}
