cmake_minimum_required(VERSION 3.20)

project(NexusFlowStreaming LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(UNIX AND NOT WIN32)

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(RDKAFKA REQUIRED IMPORTED_TARGET rdkafka)

    if(NOT TARGET RdKafka::rdkafka)
        add_library(RdKafka::rdkafka ALIAS PkgConfig::RDKAFKA)
    endif()

    message(STATUS "NEXUSFLOW STREAMING: Linux librdkafka via pkg-config")

else()

    find_package(RdKafka CONFIG REQUIRED)

endif()

add_library(
    nexusflow_streaming
    STATIC
    kafka_consumer/kafka_consumer_adapter.cpp
    kafka_consumer/kafka_pipeline_bridge.cpp
    event_replay/event_replay.cpp
    streaming_ingestion.cpp
)

target_include_directories(
    nexusflow_streaming
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(
    nexusflow_streaming
    PUBLIC
    RdKafka::rdkafka
)

add_executable(
    kafka_adapter_test
    ../tests/integration/streaming/kafka_adapter_test.cpp
)

target_link_libraries(
    kafka_adapter_test
    PRIVATE
    nexusflow_streaming
    nexusflow_core
)

add_executable(
    real_cpp_kafka_test
    ../tests/integration/streaming/real_cpp_kafka_test.cpp
)

target_link_libraries(
    real_cpp_kafka_test
    PRIVATE
    nexusflow_streaming
    nexusflow_core
)

add_executable(
    kafka_pipeline_bridge_test
    ../tests/integration/streaming/kafka_pipeline_bridge_test.cpp
)

target_include_directories(
    kafka_pipeline_bridge_test
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
)

target_link_libraries(
    kafka_pipeline_bridge_test
    PRIVATE
    nexusflow_streaming
    nexusflow_core
)

add_executable(
    kafka_pipeline_bridge_benchmark
    ../tests/integration/streaming/kafka_pipeline_bridge_benchmark.cpp
)

target_include_directories(
    kafka_pipeline_bridge_benchmark
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
)

target_link_libraries(
    kafka_pipeline_bridge_benchmark
    PRIVATE
    nexusflow_streaming
    nexusflow_core
    nexusflow_decision
)

enable_testing()

add_test(
    NAME kafka_adapter_test
    COMMAND kafka_adapter_test
)

add_test(
    NAME real_cpp_kafka_test
    COMMAND real_cpp_kafka_test
)

add_test(
    NAME kafka_pipeline_bridge_test
    COMMAND kafka_pipeline_bridge_test
)


add_executable(
    kafka_consumer_recovery_test
    ../tests/stress/kafka_consumer_recovery_test.cpp
)

target_include_directories(
    kafka_consumer_recovery_test
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
)

target_link_libraries(
    kafka_consumer_recovery_test
    PRIVATE
    nexusflow_streaming
    nexusflow_core
)

add_test(
    NAME kafka_consumer_recovery_test
    COMMAND kafka_consumer_recovery_test
)
