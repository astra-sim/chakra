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
  T get_attr(const ChakraAttr& attr, const bool strict) {
    constexpr auto STRICT_CVT = [](auto value) -> T {
      if constexpr (!std::is_same_v<T, decltype(value)>) {
        throw std::bad_cast();
      }
      return static_cast<T>(value);
    };
    constexpr auto FLAGGED_IMPLICIT_CVT = [=](auto value) -> T {
      if constexpr (std::is_integral_v<T>) {
        // integer to integer
        if constexpr (
            ALLOW_IMPLICIT_INTEGER_CONVERSION &&
            std::is_integral_v<decltype(value)>)
          return static_cast<T>(value);
        // float to integer
        if constexpr (
            ALLOW_IMPLICIT_FLOAT_TO_INTEGER_CONVERSION &&
            std::is_floating_point_v<decltype(value)>)
          return static_cast<T>(value);
      } else if constexpr (std::is_floating_point_v<T>) {
        // float to float
        if constexpr (
            ALLOW_IMPLICIT_FLOAT_CONVERSION &&
            std::is_floating_point_v<decltype(value)>)
          return static_cast<T>(value);
        // integer to float
        if constexpr (
            ALLOW_IMPLICIT_INTEGER_TO_FLOAT_CONVERSION &&
            std::is_integral_v<decltype(value)>)
          return static_cast<T>(value);
      }
      throw std::bad_cast();
    };
    constexpr auto IMPLICIT_CVT = [](auto value) -> T {
      return static_cast<T>(value);
    };
  
    // choose implicit cvt if user prefer by mod this line.
    auto cvt = strict ? STRICT_CVT : FLAGGED_IMPLICIT_CVT;
  
    switch (attr.value_case()) {
      case ChakraAttr::kDoubleVal:
        return cvt(attr.double_val());
      case ChakraAttr::kFloatVal:
        return cvt(attr.float_val());
      case ChakraAttr::kInt32Val:
        return cvt(attr.int32_val());
      case ChakraAttr::kInt64Val:
        return cvt(attr.int64_val());
      case ChakraAttr::kUint32Val:
        return cvt(attr.uint32_val());
      case ChakraAttr::kUint64Val:
        return cvt(attr.uint64_val());
      case ChakraAttr::kSint32Val:
        return cvt(attr.sint32_val());
      case ChakraAttr::kSint64Val:
        return cvt(attr.sint64_val());
      case ChakraAttr::kFixed32Val:
        return cvt(attr.fixed32_val());
      case ChakraAttr::kFixed64Val:
        return cvt(attr.fixed64_val());
      case ChakraAttr::kSfixed32Val:
        return cvt(attr.sfixed32_val());
      case ChakraAttr::kSfixed64Val:
        return cvt(attr.sfixed64_val());
      case ChakraAttr::kBoolVal:
        return cvt(attr.bool_val());
      default:
        throw std::invalid_argument("Attribute type not supported");
    }
  }

  

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
