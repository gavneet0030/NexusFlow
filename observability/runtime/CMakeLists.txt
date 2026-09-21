add_library(nexusflow_runtime_observability
    src/runtime_observability.cpp
)

target_include_directories(nexusflow_runtime_observability
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(nexusflow_runtime_observability
    PUBLIC
        nexusflow_prometheus
)

target_compile_features(nexusflow_runtime_observability
    PUBLIC cxx_std_20
)

add_executable(runtime_observability_test
    ../../tests/unit/observability/runtime_observability_test.cpp
)

target_link_libraries(runtime_observability_test
    PRIVATE
        nexusflow_runtime_observability
)

target_compile_features(runtime_observability_test
    PRIVATE cxx_std_20
)
