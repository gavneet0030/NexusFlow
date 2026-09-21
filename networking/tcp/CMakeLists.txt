add_library(nexusflow_tcp_stream
    cp_stream_session.cpp
)

target_include_directories(nexusflow_tcp_stream
    PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/networking/tcp
        ${CMAKE_SOURCE_DIR}/networking/protocol
        ${CMAKE_SOURCE_DIR}/networking/bridge
)

target_link_libraries(nexusflow_tcp_stream
    PUBLIC
        nexusflow_event_protocol
        nexusflow_tcp_pipeline_bridge
)

add_executable(tcp_persistent_stream_test
    ${CMAKE_SOURCE_DIR}/tests/integration/networking/tcp_persistent_stream_test.cpp
)

target_include_directories(tcp_persistent_stream_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/networking/tcp
        ${CMAKE_SOURCE_DIR}/networking/protocol
        ${CMAKE_SOURCE_DIR}/networking/bridge
)

target_link_libraries(tcp_persistent_stream_test
    PRIVATE
        nexusflow_tcp_stream
)

add_test(
    NAME tcp_persistent_stream_test
    COMMAND tcp_persistent_stream_test
)

add_executable(tcp_stream_robustness_test
    ${CMAKE_SOURCE_DIR}/tests/integration/networking/tcp_stream_robustness_test.cpp
)

target_include_directories(tcp_stream_robustness_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/networking/tcp
        ${CMAKE_SOURCE_DIR}/networking/protocol
        ${CMAKE_SOURCE_DIR}/networking/bridge
)

target_link_libraries(tcp_stream_robustness_test
    PRIVATE
        nexusflow_tcp_stream
)

add_test(
    NAME tcp_stream_robustness_test
    COMMAND tcp_stream_robustness_test
)

add_executable(tcp_persistent_server_test
    ${CMAKE_SOURCE_DIR}/tests/integration/networking/tcp_persistent_server_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tcp_persistent_event_server.cpp
)

target_include_directories(tcp_persistent_server_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/networking/tcp
        ${CMAKE_SOURCE_DIR}/networking/protocol
        ${CMAKE_SOURCE_DIR}/networking/bridge
)

target_link_libraries(tcp_persistent_server_test
    PRIVATE
        nexusflow_tcp_stream
        $<$<PLATFORM_ID:Windows>:ws2_32>
)

add_test(
    NAME tcp_persistent_server_test
    COMMAND tcp_persistent_server_test
)
