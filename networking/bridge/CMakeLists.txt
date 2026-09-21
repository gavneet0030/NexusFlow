add_library(nexusflow_tcp_pipeline_bridge
    tcp_pipeline_bridge.cpp
    tcp_pipeline_bridge.hpp
    tcp_core_pipeline.cpp
    tcp_core_pipeline.hpp
    tcp_adaptive_pipeline.cpp
    ${CMAKE_SOURCE_DIR}/core/processor/integrated_adaptive_pipeline.cpp
    ${CMAKE_SOURCE_DIR}/core/event/event.cpp
    ${CMAKE_SOURCE_DIR}/core/processor/event_processor.cpp
    ${CMAKE_SOURCE_DIR}/core/scheduler/scheduler_v2.cpp
    ${CMAKE_SOURCE_DIR}/core/scheduler/runtime_scheduler.cpp
    ${CMAKE_SOURCE_DIR}/core/workers/adaptive_worker_pool.cpp
    tcp_adaptive_pipeline.hpp
)

target_include_directories(nexusflow_tcp_pipeline_bridge
    PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/networking/protocol
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/core/event
        ${CMAKE_SOURCE_DIR}/core/queue
        ${CMAKE_SOURCE_DIR}/core/processor
        ${CMAKE_SOURCE_DIR}/core/scheduler
        ${CMAKE_SOURCE_DIR}/core/workers
)

target_compile_features(nexusflow_tcp_pipeline_bridge
    PUBLIC cxx_std_20
)

target_link_libraries(nexusflow_tcp_pipeline_bridge
    PUBLIC
        nexusflow_event_protocol
)

add_executable(tcp_pipeline_bridge_test
    ${CMAKE_SOURCE_DIR}/tests/integration/networking/tcp_pipeline_bridge_test.cpp
)

target_link_libraries(tcp_pipeline_bridge_test
    PRIVATE
        nexusflow_tcp_pipeline_bridge
)

target_compile_features(tcp_pipeline_bridge_test
    PRIVATE cxx_std_20
)

add_executable(tcp_core_pipeline_test
    ${CMAKE_SOURCE_DIR}/tests/integration/networking/tcp_core_pipeline_test.cpp
)

target_link_libraries(tcp_core_pipeline_test
    PRIVATE
        nexusflow_tcp_pipeline_bridge
)

target_compile_features(tcp_core_pipeline_test
    PRIVATE
        cxx_std_20
)

add_executable(tcp_adaptive_pipeline_test
    ${CMAKE_SOURCE_DIR}/tests/integration/networking/tcp_adaptive_pipeline_test.cpp
)

target_link_libraries(tcp_adaptive_pipeline_test
    PRIVATE
        nexusflow_tcp_pipeline_bridge
)

target_compile_features(tcp_adaptive_pipeline_test
    PRIVATE
        cxx_std_20
)

add_test(
    NAME tcp_pipeline_bridge_test
    COMMAND tcp_pipeline_bridge_test
)

add_test(
    NAME tcp_core_pipeline_test
    COMMAND tcp_core_pipeline_test
)

add_test(
    NAME tcp_adaptive_pipeline_test
    COMMAND tcp_adaptive_pipeline_test
)