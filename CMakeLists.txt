cmake_minimum_required(VERSION 3.20)

project(NexusFlow
    VERSION 0.1.0
    DESCRIPTION "Ultra-Low-Latency Intelligent Event Processing Platform"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

add_subdirectory(core)
add_subdirectory(streaming)
add_subdirectory(api/grpc)
add_executable(scheduler_benchmark

    benchmarks/scheduler_benchmark.cpp

    core/scheduler/fixed_scheduler.cpp    core/scheduler/adaptive_scheduler.cpp

)

target_include_directories(scheduler_benchmark
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(scheduler_integration_benchmark

    benchmarks/scheduler_integration_benchmark.cpp

    core/scheduler/fixed_scheduler.cpp

    core/scheduler/scheduler_controller.cpp    core/scheduler/adaptive_scheduler.cpp
)

target_include_directories(scheduler_integration_benchmark
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(adaptive_execution_benchmark

    benchmarks/adaptive_execution_benchmark.cpp

    core/event/event.cpp

    core/processor/batch_processor.cpp
    core/processor/adaptive_execution_engine.cpp

    core/scheduler/fixed_scheduler.cpp

    core/processor/event_processor.cpp
)

target_include_directories(adaptive_execution_benchmark
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(runtime_scheduler_benchmark

    benchmarks/runtime_scheduler_benchmark.cpp

    core/scheduler/runtime_metrics.cpp
    core/

    core/scheduler/runtime_scheduler.cpp
    core/scheduler/scheduler_v2.cpp
    core/scheduler/scheduler.cpp
)

target_include_directories(runtime_scheduler_benchmark
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(adaptive_event_pipeline_benchmark

    benchmarks/adaptive_event_pipeline_benchmark.cpp

    core/event/event.cpp

    core/processor/batch_processor.cpp
    core/processor/adaptive_execution_engine.cpp
    core/processor/adaptive_event_pipeline.cpp

    core/scheduler/runtime_metrics.cpp
    core/

    core/scheduler/runtime_scheduler.cpp
    core/scheduler/scheduler_v2.cpp
    core/scheduler/scheduler.cpp
    core/processor/event_processor.cpp
)

target_include_directories(adaptive_event_pipeline_benchmark
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(
    backpressure_benchmark
    benchmarks/backpressure_benchmark.cpp
    core/event/event.cpp

    core/processor/batch_processor.cpp
    core/processor/adaptive_execution_engine.cpp
    core/processor/adaptive_event_pipeline.cpp

    core/scheduler/runtime_metrics.cpp
    core/
    core/scheduler/fixed_scheduler.cpp

    core/scheduler/runtime_scheduler.cpp
    core/scheduler/scheduler_v2.cpp
    core/scheduler/scheduler.cpp
    core/processor/event_processor.cpp
)

target_include_directories(
    backpressure_benchmark
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if (MSVC)
    target_compile_options(
        backpressure_benchmark
        PRIVATE
        /W4
    )
endif()

add_executable(
    persistent_worker_pool_benchmark
    benchmarks/persistent_worker_pool_benchmark.cpp
    core/event/event.cpp

    core/workers/adaptive_worker_pool.cpp

)

target_include_directories(
    persistent_worker_pool_benchmark
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if (MSVC)
    target_compile_options(
        persistent_worker_pool_benchmark
        PRIVATE
        /W4
    )
endif()

add_executable(
    adaptive_worker_integration_benchmark
    benchmarks/adaptive_worker_integration_benchmark.cpp
    core/event/event.cpp
    core/processor/integrated_adaptive_pipeline.cpp



    core/

    core/workers/adaptive_worker_pool.cpp
    core/scheduler/runtime_scheduler.cpp
    core/scheduler/scheduler_v2.cpp
    core/scheduler/scheduler.cpp
)

target_include_directories(
    adaptive_worker_integration_benchmark
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if (MSVC)
    target_compile_options(
        adaptive_worker_integration_benchmark
        PRIVATE
        /W4
    )
endif()

add_executable(
    priority_scheduler_benchmark
    benchmarks/priority_scheduler_benchmark.cpp

    core/scheduler/adaptive_scheduler.cpp
)

target_include_directories(
    priority_scheduler_benchmark
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if (MSVC)
    target_compile_options(
        priority_scheduler_benchmark
        PRIVATE
        /W4
    )
endif()

add_executable(
    priority_event_queue_benchmark
    benchmarks/priority_event_queue_benchmark.cpp
    core/event/event.cpp
)

target_include_directories(
    priority_event_queue_benchmark
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if (MSVC)
    target_compile_options(
        priority_event_queue_benchmark
        PRIVATE
        /W4
    )
endif()

add_executable(
    queue_latency_experiment
    benchmarks/queue_latency_experiment.cpp
    core/event/event.cpp
)

target_include_directories(
    queue_latency_experiment
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if (MSVC)
    target_compile_options(
        queue_latency_experiment
        PRIVATE
        /W4
    )
endif()


add_executable(priority_adaptive_pipeline_benchmark
    benchmarks/priority_adaptive_pipeline_benchmark.cpp
    core/processor/priority_adaptive_pipeline.cpp

    core/scheduler/runtime_scheduler.cpp
    core/scheduler/scheduler_v2.cpp

)

target_include_directories(priority_adaptive_pipeline_benchmark PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)



add_executable(priority_latency_benchmark
    benchmarks/priority_latency_benchmark.cpp
)

target_include_directories(priority_latency_benchmark PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)


add_executable(adaptive_vs_fixed_benchmark
    benchmarks/adaptive_vs_fixed_benchmark.cpp
    core/event/event.cpp
    core/processor/integrated_adaptive_pipeline.cpp

    core/

    core/workers/adaptive_worker_pool.cpp
    core/scheduler/runtime_scheduler.cpp
    core/scheduler/scheduler_v2.cpp
    core/scheduler/scheduler.cpp

)

target_include_directories(adaptive_vs_fixed_benchmark PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)


add_executable(controlled_workload_benchmark
    benchmarks/controlled_workload_benchmark.cpp

    core/scheduler/runtime_scheduler.cpp
    core/scheduler/scheduler_v2.cpp
    core/scheduler/scheduler.cpp

)

target_include_directories(controlled_workload_benchmark PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)


add_executable(backpressure_stress_benchmark
    benchmarks/backpressure_stress_benchmark.cpp
)

target_include_directories(backpressure_stress_benchmark PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(latency_under_load_benchmark
    benchmarks/latency_under_load_benchmark.cpp
)

target_include_directories(latency_under_load_benchmark PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
add_executable(queue_comparison_benchmark
    benchmarks/queue_comparison_benchmark.cpp
)

target_include_directories(queue_comparison_benchmark PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(resource_utilization_benchmark
    benchmarks/resource_utilization_benchmark.cpp
)

target_include_directories(resource_utilization_benchmark PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
add_executable(fault_recovery_benchmark
    benchmarks/fault_recovery_benchmark.cpp
)

target_include_directories(fault_recovery_benchmark PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)



add_executable(
    controlled_adaptive_workload_benchmark
    benchmarks/controlled_adaptive_workload_benchmark.cpp
    core/event/event.cpp
    core/processor/integrated_adaptive_pipeline.cpp

    core/

    core/workers/adaptive_worker_pool.cpp
    core/scheduler/runtime_scheduler.cpp
    core/scheduler/scheduler_v2.cpp
    core/scheduler/scheduler.cpp

)
target_include_directories(
    controlled_adaptive_workload_benchmark
    PRIVATE
    ${CMAKE_SOURCE_DIR}
)
target_include_directories(
    controlled_adaptive_workload_benchmark
    PRIVATE
    ${CMAKE_SOURCE_DIR}
)






add_executable(scheduler_threshold_benchmark
    benchmarks/scheduler_threshold_benchmark.cpp

    core/scheduler/adaptive_scheduler.cpp
)

target_include_directories(
    scheduler_threshold_benchmark
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(end_to_end_threshold_benchmark
    benchmarks/end_to_end_threshold_benchmark.cpp
    core/processor/integrated_adaptive_pipeline.cpp

    core/workers/adaptive_worker_pool.cpp
    core/scheduler/runtime_scheduler.cpp
    core/scheduler/scheduler_v2.cpp
    core/scheduler/scheduler.cpp



)

target_include_directories(
    end_to_end_threshold_benchmark
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)


add_executable(
    event_replay_benchmark
    benchmarks/event_replay_benchmark.cpp
    streaming/event_replay/event_replay.cpp
)

target_include_directories(
    event_replay_benchmark
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)





# NexusFlow real dataset replay benchmark
add_executable(
    real_dataset_replay_benchmark
    benchmarks/real_dataset_replay_benchmark.cpp
    streaming/event_replay/event_replay.cpp
    core/processor/integrated_adaptive_pipeline.cpp
    core/workers/adaptive_worker_pool.cpp
    core/scheduler/runtime_scheduler.cpp
    core/scheduler/adaptive_scheduler.cpp
    core/scheduler/scheduler.cpp
    core/processor/event_processor.cpp

    core/scheduler/scheduler_v2.cpp)

target_include_directories(
    real_dataset_replay_benchmark
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
# NexusFlow real dataset benchmark matrix
add_executable(
    real_dataset_benchmark_matrix
    benchmarks/real_dataset_benchmark_matrix.cpp
    streaming/event_replay/event_replay.cpp
    core/processor/integrated_adaptive_pipeline.cpp
    core/workers/adaptive_worker_pool.cpp
    core/scheduler/runtime_scheduler.cpp
    core/scheduler/adaptive_scheduler.cpp
    core/scheduler/scheduler.cpp
    core/processor/event_processor.cpp

    core/scheduler/scheduler_v2.cpp)

target_include_directories(
    real_dataset_benchmark_matrix
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
# NexusFlow repeated real dataset benchmark
add_executable(
    real_dataset_repeated_benchmark
    benchmarks/real_dataset_repeated_benchmark.cpp
    streaming/event_replay/event_replay.cpp
    core/processor/integrated_adaptive_pipeline.cpp
    core/workers/adaptive_worker_pool.cpp
    core/scheduler/runtime_scheduler.cpp
    core/scheduler/adaptive_scheduler.cpp
    core/scheduler/scheduler.cpp
    core/processor/event_processor.cpp

    core/scheduler/scheduler_v2.cpp)

target_include_directories(
    real_dataset_repeated_benchmark
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(
    saturation_sweep_benchmark
    benchmarks/saturation_sweep_benchmark.cpp
    streaming/event_replay/event_replay.cpp
    core/processor/integrated_adaptive_pipeline.cpp
    core/workers/adaptive_worker_pool.cpp
    core/scheduler/runtime_scheduler.cpp
    core/scheduler/adaptive_scheduler.cpp
    core/scheduler/scheduler.cpp
    core/scheduler/scheduler_v2.cpp
    core/processor/event_processor.cpp
)

target_include_directories(
    saturation_sweep_benchmark
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(
    comparative_scheduler_benchmark
    benchmarks/comparative_scheduler_benchmark.cpp
    streaming/event_replay/event_replay.cpp
    core/processor/integrated_adaptive_pipeline.cpp
    core/workers/adaptive_worker_pool.cpp
    core/scheduler/runtime_scheduler.cpp
    core/scheduler/adaptive_scheduler.cpp
    core/scheduler/scheduler.cpp
    core/scheduler/scheduler_v2.cpp
    core/processor/event_processor.cpp
)

target_include_directories(
    comparative_scheduler_benchmark
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(
    end_to_end_latency_test
    benchmarks/end_to_end_latency_test.cpp
)

target_include_directories(
    end_to_end_latency_test
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)


add_executable(
    real_end_to_end_benchmark
    benchmarks/end_to_end_real_benchmark.cpp
    streaming/event_replay/event_replay.cpp
    core/processor/integrated_adaptive_pipeline.cpp
    core/workers/adaptive_worker_pool.cpp
    core/scheduler/runtime_scheduler.cpp
    core/scheduler/adaptive_scheduler.cpp
    core/scheduler/scheduler.cpp
    core/scheduler/scheduler_v2.cpp
    core/processor/event_processor.cpp
)

target_include_directories(
    real_end_to_end_benchmark
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)


add_executable(
    scheduler_v2_compile_test
    benchmarks/scheduler_v2_compile_test.cpp
    core/scheduler/scheduler_v2.cpp
    core/scheduler/scheduler.cpp
)
if(BUILD_TESTING)
    add_test(
        NAME scheduler_v2_compile_test
        COMMAND scheduler_v2_compile_test
    )
endif()

target_include_directories(
    scheduler_v2_compile_test
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)


add_executable(
    scheduler_v2_control_test
    benchmarks/scheduler_v2_control_test.cpp
    core/scheduler/scheduler.cpp
    core/scheduler/scheduler_v2.cpp
)
if(BUILD_TESTING)
    add_test(
        NAME scheduler_v2_control_test
        COMMAND scheduler_v2_control_test
    )
endif()

target_include_directories(
    scheduler_v2_control_test
    PRIVATE
    ${CMAKE_SOURCE_DIR}
)
# Scheduler V2 arrival-rate capacity scaling test
add_executable(scheduler_v2_arrival_scaling_test
    tests/unit/scheduler_v2_arrival_scaling_test.cpp
    core/scheduler/scheduler_v2.cpp
)

target_include_directories(scheduler_v2_arrival_scaling_test PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

if(BUILD_TESTING)
    add_test(
        NAME scheduler_v2_arrival_scaling_test
        COMMAND scheduler_v2_arrival_scaling_test
    )
endif()


add_executable(adaptive_capacity_sweep_benchmark
    benchmarks/adaptive_capacity_sweep_benchmark.cpp
    core/event/event.cpp
    core/processor/event_processor.cpp
    core/processor/integrated_adaptive_pipeline.cpp
    core/workers/adaptive_worker_pool.cpp
    core/scheduler/runtime_scheduler.cpp
    core/scheduler/runtime_metrics.cpp
    core/scheduler/scheduler_v2.cpp
    core/scheduler/scheduler.cpp
)

target_include_directories(adaptive_capacity_sweep_benchmark
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

if(BUILD_TESTING)
    add_test(
        NAME adaptive_capacity_sweep_benchmark
        COMMAND adaptive_capacity_sweep_benchmark
    )
endif()
add_subdirectory(storage/redis)

add_subdirectory(storage/postgres)

add_subdirectory(networking/tcp)
add_subdirectory(networking/protocol)
add_subdirectory(networking/bridge)

add_subdirectory(observability/prometheus)


add_subdirectory(observability/runtime)


add_subdirectory(observability/opentelemetry)

add_executable(opentelemetry_foundation_test
    tests/integration/observability/opentelemetry_foundation_test.cpp
)

target_link_libraries(opentelemetry_foundation_test
    PRIVATE
        nexusflow_opentelemetry
)

add_test(
    NAME opentelemetry_foundation_test
    COMMAND opentelemetry_foundation_test
)

add_executable(otlp_exporter_test
    tests/integration/observability/otlp_exporter_test.cpp
)

target_link_libraries(otlp_exporter_test
    PRIVATE
        nexusflow_opentelemetry
)

add_test(
    NAME otlp_exporter_test
    COMMAND otlp_exporter_test
)

add_executable(otlp_real_transport_test
    tests/integration/observability/otlp_real_transport_test.cpp
)

target_link_libraries(otlp_real_transport_test
    PRIVATE
        nexusflow_opentelemetry
)

add_test(
    NAME otlp_real_transport_test
    COMMAND otlp_real_transport_test
)

add_subdirectory(decision)


add_executable(nexusflow_decision_pipeline_test
    tests/integration/decision/decision_pipeline_test.cpp
)

target_include_directories(
    nexusflow_decision_pipeline_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}
)

target_compile_features(
    nexusflow_decision_pipeline_test
    PRIVATE
        cxx_std_20
)

target_link_libraries(
    nexusflow_decision_pipeline_test
    PRIVATE
        nexusflow_decision
)

include(CTest)

if(BUILD_TESTING)
    add_test(
        NAME nexusflow_decision_pipeline_test
        COMMAND nexusflow_decision_pipeline_test
    )
endif()

add_subdirectory(ml)


add_executable(nexusflow_ml_foundation_test
    tests/integration/ml/ml_foundation_test.cpp
)

target_link_libraries(nexusflow_ml_foundation_test
    PRIVATE
        nexusflow_ml
)

target_include_directories(nexusflow_ml_foundation_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/ml
)

target_compile_features(nexusflow_ml_foundation_test PRIVATE cxx_std_20)


# ============================================================
# ML Inference Benchmark
# ============================================================

add_executable(ml_inference_benchmark
    benchmarks/ml_inference_benchmark.cpp
)

target_link_libraries(ml_inference_benchmark
    PRIVATE
        nexusflow_ml
)

target_compile_features(
    ml_inference_benchmark
    PRIVATE
        cxx_std_20
)


# ============================================================
# ML Decision Integration Test
# ============================================================

add_executable(ml_decision_integration_test
    tests/integration/decision/ml_decision_integration_test.cpp
)

target_link_libraries(
    ml_decision_integration_test
    PRIVATE
        nexusflow_decision
)

target_include_directories(
    ml_decision_integration_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}/decision/decision_engine
)

target_compile_features(
    ml_decision_integration_test
    PRIVATE
        cxx_std_20
)



add_executable(
    unified_decision_pipeline_test
    tests/integration/decision/unified_decision_pipeline_test.cpp
)

target_include_directories(
    unified_decision_pipeline_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(
    unified_decision_pipeline_test
    PRIVATE
        nexusflow_decision
        nexusflow_ml
)

add_test(
    NAME unified_decision_pipeline_test
    COMMAND unified_decision_pipeline_test
)

# ============================================================
# NexusFlow Production Application
# ============================================================

add_executable(
    nexusflow
    app/main.cpp
)

target_include_directories(
    nexusflow
    PRIVATE
        ${CMAKE_SOURCE_DIR}
)

target_compile_features(
    nexusflow
    PRIVATE
        cxx_std_20
)

target_link_libraries(
    nexusflow
    PRIVATE
        nexusflow_core
)


# NexusFlow Kubernetes service executable
add_executable(
    nexusflow_service
    app/service_main.cpp
)

target_compile_features(nexusflow_service PRIVATE cxx_std_20)

target_link_libraries(
    nexusflow_service
    PRIVATE
    nexusflow_core
)

target_include_directories(
    nexusflow_service
    PRIVATE
    ${CMAKE_SOURCE_DIR}
)

add_executable(mutex_vs_lockfree_worker_pool_benchmark
    tests/benchmarks/mutex_vs_lockfree_worker_pool.cpp
)

target_link_libraries(mutex_vs_lockfree_worker_pool_benchmark
    PRIVATE
        nexusflow_core
)

add_executable(distributed_failure_recovery_test
    tests/stress/distributed_failure_recovery_test.cpp
)

target_link_libraries(distributed_failure_recovery_test
    PRIVATE
        nexusflow_core
)

add_test(
    NAME distributed_failure_recovery_test
    COMMAND distributed_failure_recovery_test
)


# NexusFlow Kafka streaming client
find_path(NEXUSFLOW_RDKAFKA_INCLUDE_DIR
    NAMES rdkafkacpp.h
    PATHS /usr/include/librdkafka
)

find_library(NEXUSFLOW_RDKAFKA_CPP_LIBRARY
    NAMES rdkafka++
)

find_library(NEXUSFLOW_RDKAFKA_LIBRARY
    NAMES rdkafka
)

if(NEXUSFLOW_RDKAFKA_INCLUDE_DIR
   AND NEXUSFLOW_RDKAFKA_CPP_LIBRARY
   AND NEXUSFLOW_RDKAFKA_LIBRARY)

    add_library(nexusflow_kafka
        streaming/kafka/kafka_client.cpp
    )

    target_include_directories(nexusflow_kafka
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${NEXUSFLOW_RDKAFKA_INCLUDE_DIR}
    )

    target_link_libraries(nexusflow_kafka
        PUBLIC
            ${NEXUSFLOW_RDKAFKA_CPP_LIBRARY}
            ${NEXUSFLOW_RDKAFKA_LIBRARY}
    )

    add_executable(cpp_kafka_integration_test
        tests/integration/streaming/kafka_producer_consumer_test.cpp
    )

    target_link_libraries(cpp_kafka_integration_test
        PRIVATE
            nexusflow_kafka
    )

    add_test(
        NAME cpp_kafka_integration_test
        COMMAND cpp_kafka_integration_test
    )

    message(STATUS "NEXUSFLOW KAFKA: librdkafka enabled")

else()

    message(FATAL_ERROR
        "NEXUSFLOW KAFKA: librdkafka development files not found"
    )

endif()


# ============================================================


