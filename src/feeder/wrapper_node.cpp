// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 Advanced Micro Devices, Inc.

#include <feeder/wrapper_node.h>

#include <algorithm>

// WrapperNode default constructor
WrapperNode::WrapperNode() {}

// WrapperNode copy constructor
WrapperNode::WrapperNode(const WrapperNode& t) {
  // Copy the attributes from the original instance to the new instance
  format_type_ = t.format_type_;
  et_feeder_ = t.et_feeder_;
  node_ = t.node_;
  protobuf_trace_base_dir_ = t.protobuf_trace_base_dir_;
  data_ = t.data_;
  num_json_nodes = t.num_json_nodes;
  json_et_complete_ = t.json_et_complete_;
  push_back_queue_proto = t.push_back_queue_proto;
  curr_json_node_id = t.curr_json_node_id;
  all_json_node_map = t.all_json_node_map;
  dep_free_json_node_ids = t.dep_free_json_node_ids;
  push_back_queue_json_node_ids = t.push_back_queue_json_node_ids;
  custom_comm_plan_map_ = t.custom_comm_plan_map_;
}

// WrapperNode constructor
WrapperNode::WrapperNode(std::string filename) {
  createWrapper(filename);
}

// Create wrapper node from a JSON/ET file
void WrapperNode::createWrapper(std::string filename) {
  std::string ext = filename.substr(filename.find_last_of(".") + 1);
  if (ext == "et") {
    std::cout << "Using Protobuf format" << std::endl;
    format_type_ = Protobuf;
    et_feeder_ = new Chakra::FeederV3::ETFeeder(filename);
    // Same base-directory idiom as v1's ETFeeder::readNode(): resolve a
    // relative custom_comm_plan_file against the trace file's own directory.
    const auto slash = filename.rfind('/');
    protobuf_trace_base_dir_ =
        (slash == std::string::npos) ? "" : filename.substr(0, slash + 1);
  } else if (ext == "json") {
    format_type_ = JSON;
    json_et_complete_ = false;
    jsonfile_.open(filename);
    data_ = json::parse(jsonfile_); // Parse JSON file
    std::cout << "Using JSON format" << std::endl;
    num_json_nodes = data_["workload_graph"].size(); // Number of nodes
    // For legacy purposes. The entire JSON file is read at once
    readAllNodes();
    resolvePlanPaths(filename);
  } else {
    std::cerr << "Error: File format not supported." << std::endl;
    exit(-1);
  }
}

// Build a one-node workload that runs the MSCCL++ plan in filename, carrying
// comm_size so the consumer can derive the plan's chunk size.
void WrapperNode::createSingleCustomCommNode(
    const std::string& filename,
    uint64_t comm_size) {
  // WrapperNode based on JSON nodes
  format_type_ = JSON;
  json_et_complete_ = false;
  num_json_nodes = 1;
  auto node_id = 0;

  // Construct a single JSON node with CUSTOM_MSCCL comm_type
  all_json_node_map[node_id] = JSONNode();
  auto& node = all_json_node_map[node_id];
  node.setID(node_id);
  node.setName("msccl_custom_comm_node");
  node.setType(JSONNode::COMM_COLL_NODE);
  node.setCommType(JSONNode::CUSTOM_MSCCL);
  node.setCustomCommPlanFile(filename);
  node.setCommSize(comm_size);
  node.setIsCPUOp(false);

  // Add this node to dep_free_json_node_ids set
  dep_free_json_node_ids.insert(node_id);
}

// Rewrite each node's custom_comm_plan_file so a plan path given relative to
// the workload file resolves correctly regardless of the process's CWD. An
// already-absolute plan path (or an empty one) is left untouched.
void WrapperNode::resolvePlanPaths(const std::string& workload_filename) {
  const auto slash = workload_filename.rfind('/');
  const std::string base_dir = (slash == std::string::npos)
      ? ""
      : workload_filename.substr(0, slash + 1);
  for (auto& entry : all_json_node_map) {
    const auto& plan = entry.second.getCustomCommPlanFile();
    if (!plan.empty() && plan.front() != '/') {
      entry.second.setCustomCommPlanFile(base_dir + plan);
    }
  }
}

// Read nodes in a window
// For JSON, the entire graph is read in a single window
void WrapperNode::readAllNodes() {
  for (auto i = 0; i < num_json_nodes; ++i) {
    JSONNode node(data_, i);
    auto node_id = node.id();
    assert(all_json_node_map.count(node_id) == 0);
    for (auto& parent_node_id : node.getDataDeps()) {
      node.addDepUnresolvedParentID(parent_node_id);
    }
    all_json_node_map[node_id] = node;
  }

  // Resolve dependencies
  resolveDep();
}

// Release memory
void WrapperNode::releaseMemory() {
  switch (format_type_) {
    case Protobuf: {
      delete et_feeder_;
      break;
    }
    case JSON: {
      jsonfile_.close();
      break;
    }
    default: {
      std::cerr << "Error in releaseMemory()" << std::endl;
      exit(-1);
    }
  }
}

WrapperNode::~WrapperNode() {}

// Remove node from dependency graph
void WrapperNode::removeNode(uint64_t node_id) {
  switch (format_type_) {
    case Protobuf: {
      et_feeder_->removeNode(node_id);
      break;
    }
    case JSON: {
      if (all_json_node_map.count(node_id) == 0) {
        std::cerr << "Trying to remove node_id=" << node_id
                  << " that does not exist" << std::endl;
        exit(-1);
      }
      all_json_node_map.erase(node_id);
      break;
    }
    default: {
      std::cerr << "Error in removeNode()" << std::endl;
      exit(-1);
    }
  }
}

// Resolve dependencies
void WrapperNode::resolveDep() {
  switch (format_type_) {
    case Protobuf: {
      // v3 resolves dependencies in the ETFeeder constructor; nothing to do
      // here.
      break;
    }
    case JSON: {
      for (auto& pair : all_json_node_map) {
        auto node_id = pair.first;
        JSONNode& node = pair.second;
        auto& dep_unresolved_parent_ids = node.getDepUnresolvedParentIDs();

        // Loop over unresolved parent IDs
        for (auto& parent_node_id : dep_unresolved_parent_ids) {
          if (all_json_node_map.count(parent_node_id) == 0) {
            std::cerr << "Parent node with id=" << parent_node_id
                      << " does not exist" << std::endl;
            exit(-1);
          }
          JSONNode& parent_node = all_json_node_map[parent_node_id];
          // Add the parent node into the data_deps list of the child node. This
          // is needed to figure out list of nodes having no data dependencies.
          node.addDataDep(parent_node);
          // Add the current node as a child to the parent
          parent_node.addChild(node);
          // std::cout << "parent_node_id=" << parent_node_id
          //           << " -> child_node_id=" << node.id()
          //           << ", data_deps_size=" << node.getDataDeps().size()
          //           << std::endl;
        }

        // All unresolved parents have been resolved, so we can clear the list
        // now
        dep_unresolved_parent_ids.clear();

        // If this node has no data dependencies, add it to the dep-free node
        // set
        if (node.getDataDeps().empty()) {
          dep_free_json_node_ids.insert(node_id);
          // std::cout << "Inserted node_id=" << node_id << " to dep-free nodes"
          //           << std::endl;
        }
      }
      break;
    }
    default: {
      std::cerr << "Error in resolveDep()" << std::endl;
      exit(-1);
    }
  }
}

// Push dependency free nodes
void WrapperNode::pushBackIssuableNode(uint64_t node_id) {
  switch (format_type_) {
    case Protobuf: {
      et_feeder_->pushBackIssuableNode(node_id);
      break;
    }
    case JSON: {
      assert(all_json_node_map.count(node_id) == 1);
      dep_free_json_node_ids.insert(node_id);
      break;
    }
    default: {
      std::cerr << "Error in pushBackIssuableNode()" << std::endl;
      exit(-1);
    }
  }
}

// Free children
void WrapperNode::freeChildrenNodes(uint64_t node_id) {
  switch (format_type_) {
    case Protobuf: {
      et_feeder_->freeChildrenNodes(node_id);
      break;
    }
    case JSON: {
      // std::cout << "Freeing children nodes of node_id=" << node_id
      //           << std::endl;
      if (all_json_node_map.count(node_id)) {
        JSONNode& node = all_json_node_map[node_id];
        for (auto& child_node_id : node.getChildren()) {
          if (all_json_node_map.count(child_node_id) == 0) {
            std::cerr << "Child node_id=" << child_node_id << " does not exist"
                      << std::endl;
            exit(-1);
          }
          JSONNode& child_node = all_json_node_map[child_node_id];
          // std::cout << "\tFreeing child node_id=" << child_node_id
          //           << ", data_deps_size=" << child_node.getDataDeps().size()
          //           << std::endl;

          // Remove this node from ths child node's data_deps list.
          auto& child_node_data_deps = child_node.getDataDeps();
          auto remove_count = child_node_data_deps.erase(node_id);
          if (remove_count != 1) {
            std::cerr << "Failed to remove node_id=" << node_id
                      << " from data_deps of node_id=" << child_node_id
                      << std::endl;
            exit(-1);
          }

          // If there is no more data dependency for this child, add it to the
          // dep-free list
          if (child_node_data_deps.empty()) {
            dep_free_json_node_ids.insert(child_node_id);
          }
        }
      } else {
        std::cerr << "freeChildrenNodes: node with id=" << node_id
                  << " does not exist" << std::endl;
        exit(-1);
      }
      break;
    }
    default: {
      std::cerr << "Error in freeChildrenNodes()" << std::endl;
      exit(-1);
    }
  }
}

// Check if the node is valid
bool WrapperNode::isValidNode() {
  switch (format_type_) {
    case Protobuf: {
      if (node_ == nullptr)
        return false;
      else
        return true;
    }
    case JSON: {
      if (all_json_node_map.count(curr_json_node_id) == 1) {
        return true;
      } else {
        return false;
      }
    }
    default: {
      std::cerr << "Error in isValid()" << std::endl;
      exit(-1);
    }
  }
}

// Push node to queue
void WrapperNode::push_to_queue() {
  switch (format_type_) {
    case Protobuf: {
      push_back_queue_proto.push(node_);
      break;
    }
    case JSON: {
      // getNextIssuableNode() already removed this node from
      // dep_free_json_node_ids; the post-loop drain in issue_dep_free_nodes()
      // puts it back via pushBackIssuableNode().
      push_back_queue_json_node_ids.push(curr_json_node_id);
      break;
    }
    default: {
      std::cerr << "Error in push_to_queue()" << std::endl;
      exit(-1);
    }
  }
}

// Check if queue is empty
bool WrapperNode::is_queue_empty() {
  switch (format_type_) {
    case Protobuf: {
      return push_back_queue_proto.empty();
    }
    case JSON: {
      return push_back_queue_json_node_ids.empty();
    }
    default: {
      std::cerr << "Error in is_queue_empty()" << std::endl;
      exit(-1);
    }
  }
}

// Get element in the queue front
void WrapperNode::queue_front() {
  switch (format_type_) {
    case Protobuf: {
      node_ = push_back_queue_proto.front();
      break;
    }
    case JSON: {
      assert(!push_back_queue_json_node_ids.empty());
      curr_json_node_id = push_back_queue_json_node_ids.front();
      break;
    }
    default: {
      std::cerr << "Error in queue_front()" << std::endl;
      exit(-1);
    }
  }
}

// Pop node from queue
void WrapperNode::pop_from_queue() {
  switch (format_type_) {
    case Protobuf: {
      push_back_queue_proto.pop();
      break;
    }
    case JSON: {
      push_back_queue_json_node_ids.pop();
      break;
    }
    default: {
      std::cerr << "Error in pop_from_queue()" << std::endl;
      exit(-1);
    }
  }
}

// Get next issuable node from dependency free queue
void WrapperNode::getNextIssuableNode() {
  switch (format_type_) {
    case Protobuf: {
      // v3's own getNextIssuableNode() dereferences begin() of a possibly
      // empty set (UB) and does not preserve v1's ascending-node-id order.
      // Select the minimum id ourselves so issue order is unchanged.
      const auto& free_nodes =
          et_feeder_->getDependancyResolver().get_dependancy_free_nodes();
      if (free_nodes.empty()) {
        node_ = nullptr;
      } else {
        const auto min_id =
            *std::min_element(free_nodes.begin(), free_nodes.end());
        et_feeder_->getDependancyResolver().take_node(min_id);
        node_ = et_feeder_->lookupNode(min_id);
      }
      break;
    }
    case JSON: {
      if (dep_free_json_node_ids.empty()) {
        curr_json_node_id = -1;
      } else {
        curr_json_node_id = *(dep_free_json_node_ids.begin());
        dep_free_json_node_ids.erase(curr_json_node_id);
      }
      break;
    }
    default: {
      std::cerr << "Error in getNextIssuableNode()" << std::endl;
      exit(-1);
    }
  }
}

// Get node ID
uint64_t WrapperNode::getNodeID() {
  switch (format_type_) {
    case Protobuf: {
      if (!isValidNode()) {
        std::cerr << "getNodeID: Protobuf node is null" << std::endl;
        exit(-1);
      }
      return node_->id();
    }
    case JSON: {
      if (!isValidNode()) {
        std::cerr << "getNodeID: Node with id=" << curr_json_node_id
                  << " does not exists" << std::endl;
        exit(-1);
      }
      return curr_json_node_id;
    }
    default: {
      std::cerr << "Error in getNodeID()" << std::endl;
      exit(-1);
    }
  }
}

// Get node name
std::string WrapperNode::getNodeName() {
  switch (format_type_) {
    case Protobuf: {
      return node_->name();
    }
    case JSON: {
      if (!isValidNode()) {
        std::cerr << "getNodeName: Node with id=" << curr_json_node_id
                  << " does not exists" << std::endl;
        exit(-1);
      }
      return all_json_node_map[curr_json_node_id].name();
    }
    default: {
      std::cerr << "Error in getNodeName()" << std::endl;
      exit(-1);
    }
  }
}

// Get node type
int WrapperNode::getNodeType() {
  switch (format_type_) {
    case Protobuf: {
      return node_->type();
    }
    case JSON: {
      if (!isValidNode()) {
        std::cerr << "getNodeType: Node with id=" << curr_json_node_id
                  << " does not exists" << std::endl;
        exit(-1);
      }
      return all_json_node_map[curr_json_node_id].type();
    }
    default: {
      std::cerr << "Error in getNodeType()" << std::endl;
      exit(-1);
    }
  }
}

// Check if CPU operation
bool WrapperNode::isCPUOp() {
  switch (format_type_) {
    case Protobuf: {
      return node_->get_attr<bool>("is_cpu_op", false);
    }
    case JSON: {
      if (!isValidNode()) {
        std::cerr << "isCPUOp: Node with id=" << curr_json_node_id
                  << " does not exists" << std::endl;
        exit(-1);
      }
      return all_json_node_map[curr_json_node_id].isCPUOp();
    }
    default: {
      std::cerr << "Error in isCPUOp()" << std::endl;
      exit(-1);
    }
  }
}

// Get runtime, always in picoseconds
uint64_t WrapperNode::getRuntime() {
  switch (format_type_) {
    case Protobuf: {
      return node_->runtime() *
          1000000ull; // protobuf runtime is in microseconds
    }
    case JSON: {
      if (!isValidNode()) {
        std::cerr << "getRuntime: Node with id=" << curr_json_node_id
                  << " does not exists" << std::endl;
        exit(-1);
      }
      return all_json_node_map[curr_json_node_id].getRuntime();
    }
    default: {
      std::cerr << "Error in getRuntime()" << std::endl;
      exit(-1);
    }
  }
}

// Get num ops
uint64_t WrapperNode::getNumOps() {
  switch (format_type_) {
    case Protobuf: {
      return node_->get_attr<uint64_t>("num_ops", 0);
    }
    case JSON: {
      if (!isValidNode()) {
        std::cerr << "getNumOps: Node with id=" << curr_json_node_id
                  << " does not exists" << std::endl;
        exit(-1);
      }
      return all_json_node_map[curr_json_node_id].getNumOps();
    }
    default: {
      std::cerr << "Error in getNumOps()" << std::endl;
      exit(-1);
    }
  }
}

// Get tensor size
uint64_t WrapperNode::getTensorSize() {
  switch (format_type_) {
    case Protobuf: {
      return node_->get_attr<uint64_t>("tensor_size", 0);
    }
    case JSON: {
      if (!isValidNode()) {
        std::cerr << "getTensorSize: Node with id=" << curr_json_node_id
                  << " does not exists" << std::endl;
        exit(-1);
      }
      return all_json_node_map[curr_json_node_id].getTensorSize();
    }
    default: {
      std::cerr << "Error in getTensorSize()" << std::endl;
      exit(-1);
    }
  }
}

// Get comm type
int64_t WrapperNode::getCommType() {
  switch (format_type_) {
    case Protobuf: {
      return node_->get_attr<int64_t>("comm_type", 0);
    }
    case JSON: {
      if (!isValidNode()) {
        std::cerr << "getCommType: Node with id=" << curr_json_node_id
                  << " does not exists" << std::endl;
        exit(-1);
      }
      return all_json_node_map[curr_json_node_id].getCommType();
    }
    default: {
      std::cerr << "Error in getCommType()" << std::endl;
      exit(-1);
    }
  }
}

// Get comm priority
uint32_t WrapperNode::getCommPriority() {
  switch (format_type_) {
    case Protobuf: {
      return node_->get_attr<uint32_t>("comm_priority", 0);
    }
    case JSON: {
      if (!isValidNode()) {
        std::cerr << "getCommPriority: Node with id=" << curr_json_node_id
                  << " does not exists" << std::endl;
        exit(-1);
      }
      return all_json_node_map[curr_json_node_id].getCommPriority();
    }
    default: {
      std::cerr << "Error in getCommPriority()" << std::endl;
      exit(-1);
    }
  }
}

// Get comm size
uint64_t WrapperNode::getCommSize() {
  switch (format_type_) {
    case Protobuf: {
      return node_->get_attr<uint64_t>("comm_size", 0);
    }
    case JSON: {
      if (!isValidNode()) {
        std::cerr << "getCommSize: Node with id=" << curr_json_node_id
                  << " does not exists" << std::endl;
        exit(-1);
      }
      return all_json_node_map[curr_json_node_id].getCommSize();
    }
    default: {
      std::cerr << "Error in getCommSize()" << std::endl;
      exit(-1);
    }
  }
}

// Get comm src
uint32_t WrapperNode::getCommSrc() {
  switch (format_type_) {
    case Protobuf: {
      return node_->get_attr<uint32_t>("comm_src", 0);
    }
    case JSON: {
      if (!isValidNode()) {
        std::cerr << "getCommSrc: Node with id=" << curr_json_node_id
                  << " does not exists" << std::endl;
        exit(-1);
      }
      return all_json_node_map[curr_json_node_id].getCommSrc();
    }
    default: {
      std::cerr << "Error in getCommSrc()" << std::endl;
      exit(-1);
    }
  }
}

// Get comm dst
uint32_t WrapperNode::getCommDst() {
  switch (format_type_) {
    case Protobuf: {
      return node_->get_attr<uint32_t>("comm_dst", 0);
    }
    case JSON: {
      if (!isValidNode()) {
        std::cerr << "getCommDst: Node with id=" << curr_json_node_id
                  << " does not exists" << std::endl;
        exit(-1);
      }
      return all_json_node_map[curr_json_node_id].getCommDst();
    }
    default: {
      std::cerr << "Error in getCommDst()" << std::endl;
      exit(-1);
    }
  }
}

// Get comm tag
uint32_t WrapperNode::getCommTag() {
  switch (format_type_) {
    case Protobuf: {
      return node_->get_attr<uint32_t>("comm_tag", 0);
    }
    case JSON: {
      if (!isValidNode()) {
        std::cerr << "getCommTag: Node with id=" << curr_json_node_id
                  << " does not exists" << std::endl;
        exit(-1);
      }
      return all_json_node_map[curr_json_node_id].getCommTag();
    }
    default: {
      std::cerr << "Error in getCommTag()" << std::endl;
      exit(-1);
    }
  }
}

// Get custom comm plan file: an installed plan map entry for this node id
// takes precedence over the node's own attribute.
std::string WrapperNode::getCustomCommPlanFile() {
  if (!isValidNode()) {
    std::cerr << "getCustomCommPlanFile: invalid node" << std::endl;
    exit(-1);
  }
  const auto it = custom_comm_plan_map_.find(getNodeID());
  if (it != custom_comm_plan_map_.end()) {
    return it->second;
  }
  switch (format_type_) {
    case Protobuf: {
      const std::string plan_file =
          node_->get_attr<std::string>("custom_comm_plan_file", "");
      // Same base-directory idiom as v1's ETFeeder::readNode(): resolve a
      // relative plan path against the trace file's own directory.
      if (!plan_file.empty() && plan_file.front() != '/') {
        return protobuf_trace_base_dir_ + plan_file;
      }
      return plan_file;
    }
    case JSON: {
      return all_json_node_map[curr_json_node_id].getCustomCommPlanFile();
    }
    default: {
      std::cerr << "Error in getCustomCommPlanFile()" << std::endl;
      exit(-1);
    }
  }
}

// Install a node id -> plan file map
void WrapperNode::setCustomCommPlanMap(
    std::unordered_map<uint64_t, std::string> plan_map) {
  custom_comm_plan_map_ = std::move(plan_map);
}

// Check if has more nodes to issue
bool WrapperNode::hasNodesToIssue() {
  switch (format_type_) {
    case Protobuf: {
      return et_feeder_->hasNodesToIssue();
    }
    case JSON: {
      return !all_json_node_map.empty();
    }
    default: {
      std::cerr << "Error in hasNodesToIssue()" << std::endl;
      exit(-1);
    }
  }
}

// Lookup Node
void WrapperNode::lookupNode(uint64_t node_id) {
  switch (format_type_) {
    case Protobuf: {
      node_ = et_feeder_->lookupNode(node_id);
      break;
    }
    case JSON: {
      if (all_json_node_map.count(node_id)) {
        curr_json_node_id = node_id;
      } else {
        std::cerr << "lookupNode: Node with id=" << node_id << " does not exist"
                  << std::endl;
        exit(-1);
      }
      break;
    }
    default: {
      std::cerr << "Error in lookupNode()" << std::endl;
      exit(-1);
    }
  }
}

JSONNode* WrapperNode::getCurrentJSONNodePtr() {
  switch (format_type_) {
    case JSON: {
      if (!isValidNode()) {
        std::cerr << "There is no valid current JSON node. curr_json_node_id="
                  << curr_json_node_id << std::endl;
        exit(-1);
      }
      return &(all_json_node_map[curr_json_node_id]);
    }
    default: {
      std::cerr << "Error in getCurrentJSONNodePtr()" << std::endl;
      exit(-1);
      return nullptr;
    }
  }
}
