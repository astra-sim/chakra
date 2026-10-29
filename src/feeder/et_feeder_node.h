// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 Advanced Micro Devices, Inc.

#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <et_def.pb.h>

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
  uint32_t comm_src_off();
  uint32_t comm_dst_off();
  const std::string& custom_comm_plan_file();
  void set_custom_comm_plan_file(const std::string& plan_file);

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

  uint64_t id_ = 0;
  std::string name_ = "";
  bool is_cpu_op_ = false;
  uint64_t runtime_ = 0;
  uint64_t num_ops_ = 0;
  uint32_t tensor_loc_ = 0;
  uint64_t tensor_size_ = 0;
  ChakraProtoMsg::CollectiveCommType comm_type_ = ChakraProtoMsg::INVALID_COMM;
  uint32_t comm_priority_ = 0;
  uint64_t comm_size_ = 0;
  uint32_t comm_src_ = 0;
  uint32_t comm_dst_ = 0;
  uint32_t comm_tag_ = 0;
  uint32_t comm_src_off_ = 0;
  uint32_t comm_dst_off_ = 0;
  std::string custom_comm_plan_file_ = "";
};

} // namespace Chakra
