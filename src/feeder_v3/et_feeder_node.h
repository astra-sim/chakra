#ifndef CHAKRA_FEEDER_V3_ET_FEEDER_NODE_H
#define CHAKRA_FEEDER_V3_ET_FEEDER_NODE_H

#include <memory>
#include "et_def.pb.h"

namespace Chakra {
namespace FeederV3 {
using NodeId = uint64_t;
using ChakraNode = ChakraProtoMsg::Node;
using ChakraAttr = ChakraProtoMsg::AttributeProto;
class ETFeeder;

class ETFeederNode {
 public:
  ETFeederNode(const ETFeeder& etfeeder, NodeId node_id)
      : feeder(etfeeder), node_id(node_id) {}

  const bool has_attr(const std::string& attr_name) const;
  const ChakraAttr get_attr_msg(const std::string& attr_name) const;
  bool get_attr_msg(const std::string& attr_name, const ChakraAttr** attr)
      const;
  const ChakraAttr::ValueCase& get_attr_type(const ChakraAttr& attr) const;
  template <typename T>
  T get_attr(const ChakraAttr& attr, const bool strict) const;
  template <typename T, typename CvtFunc>
  T dispatch_converter(const ChakraAttr& attr, CvtFunc cvt) const;

  NodeId id() const;
  std::string name() const;
  ChakraProtoMsg::NodeType type() const;
  uint64_t runtime() const;

  // old interface
  bool is_cpu_op() const;
  uint64_t num_ops() const;
  uint32_t tensor_loc() const;
  uint64_t tensor_size() const;
  ChakraProtoMsg::CollectiveCommType comm_type() const;
  uint32_t comm_priority() const;
  uint64_t comm_size() const;
  uint32_t comm_src() const;
  uint32_t comm_dst() const;
  uint32_t comm_tag() const;

 private:
  constexpr bool ALLOW_IMPLICIT_INTEGER_CONVERSION = true;
  constexpr bool ALLOW_IMPLICIT_FLOAT_CONVERSION = true;
  constexpr bool ALLOW_IMPLICIT_INTEGER_TO_FLOAT_CONVERSION = true;
  constexpr bool ALLOW_IMPLICIT_FLOAT_TO_INTEGER_CONVERSION = false;
  constexpr bool NO_IMPLICIT_CONVERSION = false;

  // ETFeederNode only store minimal thing to reduce memory usage.
  const ETfeeder& feeder;
  const NodeId node_id;

  const ChakraNode get_chakra_node() const;
}

} // namespace FeederV3
} // namespace Chakra

#endif
