#ifndef CHAKRA_FEEDER_V3_ET_FEEDER_NODE_H
#define CHAKRA_FEEDER_V3_ET_FEEDER_NODE_H

#include <memory>
#include "common.h"
#include "et_def.pb.h"

namespace Chakra {
namespace FeederV3 {
class ETFeeder;

class ETFeederNode {
 public:
  ETFeederNode(ETFeeder& etfeeder, NodeId node_id)
      : feeder(etfeeder), node_id(node_id) {}

  bool has_attr(const std::string& attr_name);
  const ChakraAttr get_attr_msg(const std::string& attr_name);
  bool get_attr_msg(const std::string& attr_name, const ChakraAttr** attr);
  ChakraAttr::ValueCase get_attr_type(const ChakraAttr& attr);
  template <typename T>
  T get_attr(const ChakraAttr& attr, const bool strict);

  const NodeId id() const ;
  const std::string name() const ;
  const ChakraProtoMsg::NodeType type() const ;
  const uint64_t runtime() const ;

  // old interface
  bool is_cpu_op();
  uint64_t num_ops();
  uint32_t tensor_loc();
  uint64_t tensor_size();
  ChakraProtoMsg::CollectiveCommType comm_type();
  uint32_t comm_priority();
  uint64_t comm_size();
  uint32_t comm_src();
  uint32_t comm_dst();
  uint32_t comm_tag();

 private:
  // ETFeederNode only store minimal thing to reduce memory usage.
  ETFeeder& feeder;
  NodeId node_id;
  mutable std::weak_ptr<const ChakraNode> chakra_node;

  std::shared_ptr<const ChakraNode> get_chakra_node() const ;
};

} // namespace FeederV3
} // namespace Chakra

#endif
