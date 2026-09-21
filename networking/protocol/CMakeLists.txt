add_library(nexusflow_event_protocol
    event_protocol.cpp
    event_protocol.hpp
)

target_include_directories(nexusflow_event_protocol
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(nexusflow_event_protocol
    PUBLIC cxx_std_20
)

add_executable(event_protocol_test
    ${CMAKE_SOURCE_DIR}/tests/integration/networking/event_protocol_test.cpp
)

target_link_libraries(event_protocol_test
    PRIVATE
        nexusflow_event_protocol
)

target_compile_features(event_protocol_test
    PRIVATE cxx_std_20
)

add_test(
    NAME event_protocol_test
    COMMAND event_protocol_test
)