#ifndef CHAKRA_FEEDER_V3_PROTOBUF_UTIL_H
#define CHAKRA_FEEDER_V3_PROTOBUF_UTIL_H

#include <cstdint>
#include <iostream>

namespace Chakra {
namespace FeederV3 {
class ProtobufUtils {
 public:
  static bool readVarint32(std::istream& f, uint32_t& value);

  template <typename T>
  static bool readMessage(std::istream& f, T& msg);

  static bool writeVarint32(std::ostream& f, uint32_t value);

  template <typename T>
  static bool writeMessage(std::ostream& f, const T& msg);
};
} // namespace FeederV3
} // namespace Chakra
#endif
