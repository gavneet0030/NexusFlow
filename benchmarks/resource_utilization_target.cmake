add_executable(resource_utilization_benchmark
    benchmarks/resource_utilization_benchmark.cpp
)

target_include_directories(resource_utilization_benchmark PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
