#ifndef CHAKRA_FEEDER_V3_COMMON_H
#define CHAKRA_FEEDER_V3_COMMON_H
#include <cstdint>
#include "et_def.pb.h"

namespace Chakra {
namespace FeederV3 {
using NodeId = uint64_t;
using ETFeederId = uint64_t;
using ChakraNode = ChakraProtoMsg::Node;
using ChakraGlobalMetadata = ChakraProtoMsg::GlobalMetadata;    
using ChakraAttr = ChakraProtoMsg::AttributeProto;

constexpr static bool ALLOW_IMPLICIT_INTEGER_CONVERSION = true;
constexpr static bool ALLOW_IMPLICIT_FLOAT_CONVERSION = true;
constexpr static bool ALLOW_IMPLICIT_INTEGER_TO_FLOAT_CONVERSION = true;
constexpr static bool ALLOW_IMPLICIT_FLOAT_TO_INTEGER_CONVERSION = false;
constexpr static bool NO_IMPLICIT_CONVERSION = false;

constexpr static size_t DEFAULT_CACHE_SIZE = 16384;
constexpr static bool USE_DATA_DEPS = true;
constexpr static bool USE_CTRL_DEPS = true;
} // namespace FeederV3
} // namespace Chakra

#endif
