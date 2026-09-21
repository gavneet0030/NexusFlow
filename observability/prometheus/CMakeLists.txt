add_library(nexusflow_prometheus
    src/prometheus_metrics.cpp
    src/metrics_http_server.cpp
)

target_include_directories(nexusflow_prometheus
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(nexusflow_prometheus
    PUBLIC cxx_std_20
)

if (MSVC)
    target_compile_options(nexusflow_prometheus
        PRIVATE /W4
    )

    target_link_libraries(nexusflow_prometheus
        PRIVATE
            $<$<PLATFORM_ID:Windows>:ws2_32>
    )
endif()

add_executable(prometheus_metrics_test
    ../../tests/unit/observability/prometheus_metrics_test.cpp
)

target_link_libraries(prometheus_metrics_test
    PRIVATE
        nexusflow_prometheus
)

target_compile_features(prometheus_metrics_test
    PRIVATE cxx_std_20

)

add_executable(metrics_http_server_test
    ../../tests/unit/observability/metrics_http_server_test.cpp
)

target_link_libraries(metrics_http_server_test
    PRIVATE
        nexusflow_prometheus
)

target_compile_features(metrics_http_server_test
    PRIVATE cxx_std_20
)
