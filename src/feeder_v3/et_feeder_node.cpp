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
