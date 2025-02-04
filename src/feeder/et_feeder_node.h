#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <optional>

#include "et_def.pb.h"

namespace Chakra {

class ETFeederNode {
 public:
  ETFeederNode(std::shared_ptr<ChakraProtoMsg::Node> node);
  std::shared_ptr<ChakraProtoMsg::Node> getChakraNode();
  void addChild(std::shared_ptr<ETFeederNode> node);
  std::vector<std::shared_ptr<ETFeederNode>> getChildren();
  void addDepUnresolvedParentID(uint64_t node_id);
  std::vector<uint64_t> getDepUnresolvedParentIDs();
  void setDepUnresolvedParentIDs(
      std::vector<uint64_t> const& dep_unresolved_parent_ids);

  const ChakraProtoMsg::AttributeProto& get_other_attr(
      const std::string& attr_name) const;
  bool has_other_attr(const std::string& attr_name) const;

  uint64_t id();
  std::string name();
  bool is_cpu_op();
  ChakraProtoMsg::NodeType type();
  uint64_t runtime();
  uint64_t num_ops();
  uint32_t tensor_loc();
  uint64_t tensor_size();
  ChakraProtoMsg::CollectiveCommType comm_type();
  uint32_t comm_priority();
  uint64_t comm_size();
  uint32_t comm_src();
  uint32_t comm_dst();
  uint32_t comm_tag();
  std::string pg_name();

 private:
  void assign_attr_val(
      std::shared_ptr<ChakraProtoMsg::Node> node,
      int i,
      void* member);

  std::shared_ptr<ChakraProtoMsg::Node> node_{nullptr};
  std::unordered_set<std::shared_ptr<ETFeederNode>> children_set_{};
  std::vector<std::shared_ptr<ETFeederNode>> children_vec_{};
  std::vector<uint64_t> dep_unresolved_parent_ids_{};
  std::unordered_map<std::string, const ChakraProtoMsg::AttributeProto&>
      other_attrs_{};

  // required metadata
  uint64_t id_;
  std::string name_;
  bool is_cpu_op_;
  uint64_t runtime_;

  // optional metadata
  std::optional<uint64_t> num_ops_;
  std::optional<uint32_t> tensor_loc_;
  std::optional<uint64_t> tensor_size_;
  std::optional<ChakraProtoMsg::CollectiveCommType> comm_type_;
  std::optional<uint32_t> comm_priority_;
  std::optional<uint64_t> comm_size_;
  std::optional<uint32_t> comm_src_;
  std::optional<uint32_t> comm_dst_;
  std::optional<uint32_t> comm_tag_;
  std::optional<std::string> pg_name_;
};

} // namespace Chakra
