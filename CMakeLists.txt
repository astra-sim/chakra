# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2026 Advanced Micro Devices, Inc.

add_library(ChakraEtFeeder)

set(SCHEMA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/schema/protobuf)
set(FEEDER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/feeder)

# Compile Protobuf schema
# Use CONFIG mode to link to protobuf::libprotobuf
find_package(protobuf CONFIG REQUIRED)

set(PROTO_SOURCES "${SCHEMA_DIR}/et_def.proto")
protobuf_generate(
    TARGET ChakraEtFeeder
    LANGUAGE cpp
    PROTOS ${PROTO_SOURCES}
    IMPORT_DIRS ${SCHEMA_DIR}
    PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR})

set(FEEDER_V3_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/feeder_v3)

set (FEEDER_SOURCES
    ${FEEDER_V3_DIR}/et_feeder.cpp
    ${FEEDER_V3_DIR}/et_feeder_node.cpp
    ${FEEDER_V3_DIR}/dependancy_solver.cpp
    ${FEEDER_DIR}/wrapper_node.cpp
    ${FEEDER_DIR}/json_node.cpp
    )

# Link libraries
target_link_libraries(ChakraEtFeeder PUBLIC
                      protobuf::libprotobuf
                      nlohmann_json::nlohmann_json)

target_include_directories(ChakraEtFeeder
    PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    src)

target_sources(ChakraEtFeeder
    PRIVATE
    ${FEEDER_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/utils/protoio.cc)

set_target_properties(ChakraEtFeeder
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)