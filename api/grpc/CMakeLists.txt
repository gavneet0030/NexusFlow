cmake_minimum_required(VERSION 3.20)

# ------------------------------------------------------------
# Platform-specific gRPC and Abseil configuration
# ------------------------------------------------------------

if (WIN32)
    set(NEXUSFLOW_GRPC_PLATFORM_LIBS "")
else()
    find_package(absl CONFIG REQUIRED)

    set(NEXUSFLOW_GRPC_PLATFORM_LIBS
        absl::base
        absl::strings
        absl::status
        absl::statusor
        absl::synchronization
    )
endif()

# ------------------------------------------------------------
# Generated protobuf files
# ------------------------------------------------------------

set(GRPC_GENERATED_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/generated
)

set(GRPC_PROTO
    ${CMAKE_CURRENT_SOURCE_DIR}/nexusflow.proto
)

set(GRPC_PROTO_SRCS
    ${GRPC_GENERATED_DIR}/nexusflow.pb.cc
    ${GRPC_GENERATED_DIR}/nexusflow.grpc.pb.cc
)

set(GRPC_PROTO_HDRS
    ${GRPC_GENERATED_DIR}/nexusflow.pb.h
    ${GRPC_GENERATED_DIR}/nexusflow.grpc.pb.h
)

# ------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------

# NexusFlow Windows dependency discovery.
# Keep Protobuf, gRPC and Abseil on the MSYS2 UCRT64 toolchain.

if(WIN32)
    # MSYS2 UCRT64 dependency stack.
    # Keep Protobuf, gRPC and Abseil on the same toolchain.
    set(NEXUSFLOW_UCRT64_PREFIX "C:/msys64/ucrt64")

    set(Protobuf_DIR
        "${NEXUSFLOW_UCRT64_PREFIX}/lib/cmake/protobuf"
        CACHE PATH "Windows Protobuf package directory"
    )

    set(gRPC_DIR
        "${NEXUSFLOW_UCRT64_PREFIX}/lib/cmake/grpc"
        CACHE PATH "Windows gRPC package directory"
    )

    set(absl_DIR
        "${NEXUSFLOW_UCRT64_PREFIX}/lib/cmake/absl"
        CACHE PATH "Windows Abseil package directory"
    )

    find_package(absl CONFIG REQUIRED)
    find_package(Protobuf CONFIG REQUIRED)
    find_package(gRPC CONFIG REQUIRED)

    set(NEXUSFLOW_UCRT64_BIN
        "${NEXUSFLOW_UCRT64_PREFIX}/bin"
    )

    set(NEXUSFLOW_GRPC_PLATFORM_LIBS
        absl::base
        absl::strings
        absl::status
        absl::statusor
        absl::synchronization
    )
else()
    unset(Protobuf_DIR CACHE)
    unset(gRPC_DIR CACHE)
    unset(re2_DIR CACHE)
    unset(absl_DIR CACHE)

    find_package(Protobuf REQUIRED)
    find_package(gRPC CONFIG REQUIRED)
    find_package(absl CONFIG REQUIRED)
    
# NexusFlow Linux RE2 discovery.
#
# Ubuntu exposes RE2 through pkg-config rather than a re2Config.cmake
# package configuration file. Create the canonical re2::re2 target
# expected by the gRPC dependency graph.

if(UNIX AND NOT WIN32)

    find_package(PkgConfig REQUIRED)

    pkg_check_modules(NEXUSFLOW_RE2 REQUIRED IMPORTED_TARGET re2)

    if(NOT TARGET re2::re2)
        add_library(re2::re2 ALIAS PkgConfig::NEXUSFLOW_RE2)
    endif()

    message(STATUS
        "NEXUSFLOW RE2: Linux pkg-config"
    )

else()

    find_package(re2 CONFIG REQUIRED)

endif()

endif()

# Abseil is provided by the selected platform dependency stack.
if (NOT WIN32)
    find_package(absl CONFIG REQUIRED)
endif()

# ------------------------------------------------------------
# Generated gRPC library
# ------------------------------------------------------------

add_library(nexusflow_grpc_generated
    ${GRPC_PROTO_SRCS}
)

target_include_directories(nexusflow_grpc_generated
    PUBLIC
        ${GRPC_GENERATED_DIR}
)

target_link_libraries(nexusflow_grpc_generated
    PUBLIC
        protobuf::libprotobuf
        gRPC::grpc++
        ${NEXUSFLOW_GRPC_PLATFORM_LIBS}
)

# ------------------------------------------------------------
# NexusFlow gRPC service
# ------------------------------------------------------------

add_library(nexusflow_grpc_service
    nexusflow_grpc_service.cpp
    nexusflow_grpc_server.cpp

    ${CMAKE_SOURCE_DIR}/core/processor/integrated_adaptive_pipeline.cpp
    ${CMAKE_SOURCE_DIR}/core/event/event.cpp
    ${CMAKE_SOURCE_DIR}/core/processor/event_processor.cpp
    ${CMAKE_SOURCE_DIR}/core/scheduler/scheduler_v2.cpp
    ${CMAKE_SOURCE_DIR}/core/scheduler/runtime_scheduler.cpp
    ${CMAKE_SOURCE_DIR}/core/workers/adaptive_worker_pool.cpp
)

target_include_directories(nexusflow_grpc_service
    PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${GRPC_GENERATED_DIR}
)

target_link_libraries(nexusflow_grpc_service
    PUBLIC
        nexusflow_grpc_generated
        gRPC::grpc++
        protobuf::libprotobuf
        ${NEXUSFLOW_GRPC_PLATFORM_LIBS}
)

# ------------------------------------------------------------
# gRPC core pipeline integration test
# ------------------------------------------------------------

add_executable(grpc_core_pipeline_test
    ${CMAKE_SOURCE_DIR}/tests/integration/grpc/grpc_core_pipeline_test.cpp
)

target_include_directories(grpc_core_pipeline_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${GRPC_GENERATED_DIR}
)

target_link_libraries(grpc_core_pipeline_test
    PRIVATE
        nexusflow_grpc_service
        gRPC::grpc++
        protobuf::libprotobuf
        ${NEXUSFLOW_GRPC_PLATFORM_LIBS}
)

# ------------------------------------------------------------
# gRPC real transport test
# ------------------------------------------------------------

add_executable(grpc_transport_test
    ${CMAKE_SOURCE_DIR}/tests/integration/grpc/grpc_transport_test.cpp
)

target_include_directories(grpc_transport_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${GRPC_GENERATED_DIR}
)

target_link_libraries(grpc_transport_test
    PRIVATE
        nexusflow_grpc_service
        gRPC::grpc++
        protobuf::libprotobuf
        ${NEXUSFLOW_GRPC_PLATFORM_LIBS}
)

# ------------------------------------------------------------
# Windows UCRT64 runtime
# ------------------------------------------------------------
#
# gRPC/Protobuf/Abseil are provided by the MSYS2 UCRT64
# dependency stack and are linked through their CMake targets.
# No manual DLL staging is required.

# ------------------------------------------------------------
# CTest registration
# ------------------------------------------------------------

add_test(
    NAME grpc_core_pipeline_test
    COMMAND grpc_core_pipeline_test
)

add_test(
    NAME grpc_transport_test
    COMMAND grpc_transport_test
)

# ------------------------------------------------------------
# Configuration diagnostics
# ------------------------------------------------------------

if (WIN32)
    message(STATUS "NEXUSFLOW gRPC PLATFORM: WINDOWS")
    message(STATUS "NEXUSFLOW gRPC RUNTIME: MSYS2 UCRT64")
else()
    message(STATUS "NEXUSFLOW gRPC PLATFORM: LINUX")
    message(STATUS "NEXUSFLOW gRPC RUNTIME: SYSTEM")
endif()

message(STATUS "NEXUSFLOW gRPC CMAKE: CLEAN")







# NEXUSFLOW_LINUX_DEPENDENCY_SANITIZATION
#
# The Windows development environment may contain unrelated third-party
# include directories. Imported dependency targets can propagate those
# directories into Linux builds through their INTERFACE_INCLUDE_DIRECTORIES.
#
# Linux must use only the system Ubuntu protobuf/gRPC installation.

if(UNIX AND NOT WIN32)

    function(nexusflow_strip_windows_dependency_includes target_name)

        if(TARGET ${target_name})

            get_target_property(
                _nexusflow_includes
                ${target_name}
                INTERFACE_INCLUDE_DIRECTORIES
            )

            if(_nexusflow_includes)

                set(_nexusflow_clean_includes "")

                foreach(_nexusflow_include IN LISTS _nexusflow_includes)

                    if(NOT _nexusflow_include MATCHES "anaconda3")
                        list(APPEND
                            _nexusflow_clean_includes
                            "${_nexusflow_include}"
                        )
                    endif()

                endforeach()

                set_target_properties(
                    ${target_name}
                    PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES
                    "${_nexusflow_clean_includes}"
                )

            endif()

        endif()

    endfunction()

    nexusflow_strip_windows_dependency_includes(gRPC::grpc++)
    nexusflow_strip_windows_dependency_includes(gRPC::grpc++_unsecure)
    nexusflow_strip_windows_dependency_includes(gRPC::grpc)
    nexusflow_strip_windows_dependency_includes(gRPC::grpc_unsecure)

    nexusflow_strip_windows_dependency_includes(protobuf::libprotobuf)
    nexusflow_strip_windows_dependency_includes(protobuf::libprotobuf-lite)
    nexusflow_strip_windows_dependency_includes(protobuf::libprotoc)

endif()

