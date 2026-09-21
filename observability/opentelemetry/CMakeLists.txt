add_library(nexusflow_opentelemetry
    src/trace_context.cpp
    src/otlp_exporter.cpp
    generated/trace.pb.cc
    generated/trace_service.pb.cc
)

target_include_directories(nexusflow_opentelemetry
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/generated
)

target_compile_features(nexusflow_opentelemetry
    PUBLIC
        cxx_std_20
)

# Keep OpenTelemetry on the same Protobuf package used by gRPC.
if(WIN32)
    set(NEXUSFLOW_UCRT64_PREFIX "C:/msys64/ucrt64")

    set(Protobuf_DIR
        "${NEXUSFLOW_UCRT64_PREFIX}/lib/cmake/protobuf"
        CACHE PATH "NexusFlow Protobuf CMake package directory"
    )

    set(absl_DIR
        "${NEXUSFLOW_UCRT64_PREFIX}/lib/cmake/absl"
        CACHE PATH "NexusFlow Abseil package directory"
    )

    find_package(absl CONFIG REQUIRED)
    find_package(Protobuf CONFIG REQUIRED)
else()
    find_package(Protobuf REQUIRED)
endif()

if (TARGET protobuf::libprotobuf)
    target_link_libraries(
        nexusflow_opentelemetry
        PUBLIC
            protobuf::libprotobuf
    )
elseif (TARGET protobuf::libprotobuf-lite)
    target_link_libraries(
        nexusflow_opentelemetry
        PUBLIC
            protobuf::libprotobuf-lite
    )
else()
    message(FATAL_ERROR
        "No compatible protobuf CMake target found"
    )
endif()

if (WIN32)
    target_link_libraries(
        nexusflow_opentelemetry
        PUBLIC
            winhttp
    )
else()
    find_package(CURL REQUIRED)

    target_link_libraries(
        nexusflow_opentelemetry
        PUBLIC
            CURL::libcurl
    )
endif()




