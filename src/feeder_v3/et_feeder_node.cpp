#include "et_feeder_node.h"
#include "et_feeder.h"

using namespace Chakra::FeederV3;

std::shared_ptr<const ChakraNode> ETFeederNode::get_chakra_node() const {
  if (this->chakra_node.expired()) {
    auto node = this->feeder.get_raw_chakra_node(this->node_id);
    this->chakra_node = node;
    return node;
  }
  return this->chakra_node.lock();
}

bool ETFeederNode::has_attr(const std::string& attr_name) {
  const auto node = this->get_chakra_node();
  for (auto& attr : node->attr())
    if (attr.name() == attr_name)
      return true;
  return false;
}

const ChakraAttr ETFeederNode::get_attr_msg(const std::string& attr_name) {
  const auto node = this->get_chakra_node();
  for (auto& attr : node->attr())
    if (attr.name() == attr_name)
      return attr;
  throw std::runtime_error(
      "Attribute " + attr_name + " not found in node " +
      std::to_string(this->node_id));
}

bool ETFeederNode::get_attr_msg(const std::string& attr_name, const ChakraAttr** attr) {
  const auto node = this->get_chakra_node();
  for (auto& iter_attr : node->attr())
    if (iter_attr.name() == attr_name) {
      *attr = &iter_attr;
      return true;
    }
  return false;
}

ChakraAttr::ValueCase ETFeederNode::get_attr_type(const ChakraAttr& attr) {
  return attr.value_case();
}

// template <typename T>
// T ETFeederNode::get_attr_strict_typed( ChakraAttr& attr)  {
//   auto get_value = [](auto value, auto expected_type) -> T {
//     if expr (!std::is_same<T, decltype(expected_type)>::value) {
//       throw std::bad_cast("Attribute type not supported");
//     }
//     return static_cast<T>(value);
//   };
//   switch (attr.value_case()) {
//     case ChakraAttr::kDoubleVal:
//       return get_value(attr.double_val(), double());
//     case ChakraAttr::kFloatVal:
//       return get_value(attr.float_val(), float());
//     case ChakraAttr::kInt32Val:
//       return get_value(attr.int32_val(), int32_t());
//     case ChakraAttr::kInt64Val:
//       return get_value(attr.int64_val(), int64_t());
//     case ChakraAttr::kUint32Val:
//       return get_value(attr.uint32_val(), uint32_t());
//     case ChakraAttr::kUint64Val:
//       return get_value(attr.uint64_val(), uint64_t());
//     case ChakraAttr::kSint32Val:
//       return get_value(attr.sint32_val(), int32_t());
//     case ChakraAttr::kSint64Val:
//       return get_value(attr.sint64_val(), int64_t());
//     case ChakraAttr::kFixed32Val:
//       return get_value(attr.fixed32_val(), uint32_t());
//     case ChakraAttr::kFixed64Val:
//       return get_value(attr.fixed64_val(), uint64_t());
//     case ChakraAttr::kSfixed32Val:
//       return get_value(attr.sfixed32_val(), int32_t());
//     case ChakraAttr::kSfixed64Val:
//       return get_value(attr.sfixed64_val(), int64_t());
//     case ChakraAttr::kBoolVal:
//       return get_value(attr.bool_val(), bool());
//     default:
//       throw std::bad_cast("Attribute type not supported");
//   }
// }

// template <typename T>
// T ETFeederNode::get_attr( ChakraAttr& attr)  {
//   try {
//     return this->get_attr_strict_typed<T>(attr);
//   } catch ( std::bad_cast& e) {
//     if expr (this->NO_IMPLICIT_CONVERSION)
//       goto BAD_CAST;
//   }

//    auto& attr_type = this->get_attr_type(attr);
//   if expr (std::is_integral_v<T>) {
//     if expr (this->ALLOW_IMPLICIT_INTEGER_CONVERSION) {
//       switch (attr_type) {
//         case ChakraAttr::kInt32Val:
//           return std::static_cast<T>(attr.int32_val());
//         case ChakraAttr::kInt64Val:
//           return std::static_cast<T>(attr.int64_val());
//         case ChakraAttr::kUint32Val:
//           return std::static_cast<T>(attr.uint32_val());
//         case ChakraAttr::kUint64Val:
//           return std::static_cast<T>(attr.uint64_val());
//         case ChakraAttr::kSint32Val:
//           return std::static_cast<T>(attr.sint32_val());
//         case ChakraAttr::kSint64Val:
//           return std::static_cast<T>(attr.sint64_val());
//         case ChakraAttr::kFixed32Val:
//           return std::static_cast<T>(attr.fixed32_val());
//         case ChakraAttr::kFixed64Val:
//           return std::static_cast<T>(attr.fixed64_val());
//         case ChakraAttr::kSfixed32Val:
//           return std::static_cast<T>(attr.sfixed32_val());
//         case ChakraAttr::kSfixed64Val:
//           return std::static_cast<T>(attr.sfixed64_val());
//         case ChakraAttr::kBoolVal:
//           return std::static_cast<T>(attr.bool_val());
//         default:
//           break; // fall back to float_to_integer conversion
//       }
//     }
//     if expr (this->ALLOW_IMPLICIT_FLOAT_TO_INTEGER_CONVERSION) {
//       switch (attr_type) {
//         case ChakraAttr::kDoubleVal:
//           return std::static_cast<T>(attr.double_val());
//         case ChakraAttr::kFloatVal:
//           return std::static_cast<T>(attr.float_val());
//         default:
//           break; // fall back to bad cast
//       }
//     }
//   } else if expr (std::is_floating_point_v<T>) {
//     if expr (this->ALLOW_IMPLICIT_FLOAT_CONVERSION) {
//       switch (attr_type) {
//         case ChakraAttr::kDoubleVal:
//           return std::static_cast<T>(attr.double_val());
//         case ChakraAttr::kFloatVal:
//           return std::static_cast<T>(attr.float_val());
//         default:
//           break; // fall back to integer_to_float conversion
//       }
//     }
//     if expr (this->ALLOW_IMPLICIT_INTEGER_TO_FLOAT_CONVERSION) {
//       switch (attr_type) {
//         case ChakraAttr::kInt32Val:
//           return std::static_cast<T>(attr.int32_val());
//         case ChakraAttr::kInt64Val:
//           return std::static_cast<T>(attr.int64_val());
//         case ChakraAttr::kUint32Val:
//           return std::static_cast<T>(attr.uint32_val());
//         case ChakraAttr::kUint64Val:
//           return std::static_cast<T>(attr.uint64_val());
//         case ChakraAttr::kSint32Val:
//           return std::static_cast<T>(attr.sint32_val());
//         case ChakraAttr::kSint64Val:
//           return std::static_cast<T>(attr.sint64_val());
//         case ChakraAttr::kFixed32Val:
//           return std::static_cast<T>(attr.fixed32_val());
//         case ChakraAttr::kFixed64Val:
//           return std::static_cast<T>(attr.fixed64_val());
//         case ChakraAttr::kSfixed32Val:
//           return std::static_cast<T>(attr.sfixed32_val());
//         case ChakraAttr::kSfixed64Val:
//           return std::static_cast<T>(attr.sfixed64_val());
//         case ChakraAttr::kBoolVal:
//           return std::static_cast<T>(attr.bool_val());
//         default:
//           break; // fall back to bad cast
//       }
//     }
//   }

// BAD_CAST:
//   throw std::bad_cast("Attribute type not supported");
// }

template <typename T>
T ETFeederNode::get_attr(const ChakraAttr& attr, const bool strict) {
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

const NodeId ETFeederNode::id() const {
  auto node = this->get_chakra_node();
  return node->id();
}

const std::string ETFeederNode::name() const {
  auto node = this->get_chakra_node();
  return node->name();
}

const ChakraProtoMsg::NodeType ETFeederNode::type() const {
  auto node = this->get_chakra_node();
  return node->type();
}

const uint64_t ETFeederNode::runtime() const {
  auto node = this->get_chakra_node();
  return node->duration_micros();
}
