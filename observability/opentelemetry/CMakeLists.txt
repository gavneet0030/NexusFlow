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
if(WIN32 AND MSVC)
    # Native MSVC: use the matching Anaconda dependencies.
    set(NEXUSFLOW_NATIVE_MSVC_PREFIX "C:/Users/PC/anaconda3/Library")

    set(Protobuf_DIR
        "${NEXUSFLOW_NATIVE_MSVC_PREFIX}/lib/cmake/protobuf"
        CACHE PATH "NexusFlow Protobuf CMake package directory" FORCE
    )
    set(absl_DIR
        "${NEXUSFLOW_NATIVE_MSVC_PREFIX}/lib/cmake/absl"
        CACHE PATH "NexusFlow Abseil package directory" FORCE
    )

    find_package(absl CONFIG REQUIRED)
    find_package(Protobuf CONFIG REQUIRED)

    # Keep MSYS2 include paths out of native MSVC compilation.
    foreach(_nexusflow_absl_target IN ITEMS
        absl::absl_check
        absl::absl_log
        absl::algorithm
        absl::base
        absl::bind_front
        absl::bits
        absl::btree
        absl::cleanup
        absl::cord
        absl::core_headers
        absl::debugging
        absl::die_if_null
        absl::dynamic_annotations
        absl::flags
        absl::flat_hash_map
        absl::flat_hash_set
        absl::function_ref
        absl::hash
        absl::layout
        absl::log_initialize
        absl::log_globals
        absl::log_severity
        absl::memory
        absl::node_hash_map
        absl::node_hash_set
        absl::optional
        absl::random_distributions
        absl::random_random
        absl::span
        absl::status
        absl::statusor
        absl::strings
        absl::synchronization
        absl::time
        absl::utility
    )
        if(TARGET "${_nexusflow_absl_target}")
            get_target_property(_nexusflow_absl_includes
                "${_nexusflow_absl_target}"
                INTERFACE_INCLUDE_DIRECTORIES
            )
            if(_nexusflow_absl_includes)
                list(FILTER _nexusflow_absl_includes
                    EXCLUDE REGEX "^C:/msys64/ucrt64/include$"
                )
                set_target_properties("${_nexusflow_absl_target}" PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${_nexusflow_absl_includes}"
                )
            endif()
        endif()
    endforeach()

    if(TARGET protobuf::libprotobuf)
        get_target_property(_nexusflow_protobuf_includes
            protobuf::libprotobuf INTERFACE_INCLUDE_DIRECTORIES
        )
        if(_nexusflow_protobuf_includes)
            list(FILTER _nexusflow_protobuf_includes
                EXCLUDE REGEX "^C:/msys64/ucrt64/include$"
            )
            set_target_properties(protobuf::libprotobuf PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${_nexusflow_protobuf_includes}"
            )
        endif()
    endif()

elseif(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
       AND CMAKE_CXX_COMPILER MATCHES "msys64")
    # MSYS2 UCRT64 GCC: use Protobuf and Abseil from the same toolchain.
    set(Protobuf_DIR "C:/msys64/ucrt64/lib/cmake/protobuf"
        CACHE PATH "NexusFlow MSYS2 Protobuf package directory" FORCE
    )
    set(absl_DIR "C:/msys64/ucrt64/lib/cmake/absl"
        CACHE PATH "NexusFlow MSYS2 Abseil package directory" FORCE
    )

    find_package(absl CONFIG REQUIRED)
    find_package(Protobuf CONFIG REQUIRED)

else()
    # Linux and other supported non-MSVC platforms.
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

# MSVC consumer sanitization.
# Do not allow the MSYS2 UCRT64 system include directory to become
# an include directory of the NexusFlow OpenTelemetry target.
if (MSVC)
    set_target_properties(nexusflow_opentelemetry PROPERTIES
        INCLUDE_DIRECTORIES
        "${CMAKE_CURRENT_SOURCE_DIR}/include;${CMAKE_CURRENT_SOURCE_DIR}/generated"
        INTERFACE_INCLUDE_DIRECTORIES
        "${CMAKE_CURRENT_SOURCE_DIR}/include;${CMAKE_CURRENT_SOURCE_DIR}/generated"
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ""
    )

    get_property(_nf_ot_includes TARGET nexusflow_opentelemetry
        PROPERTY INCLUDE_DIRECTORIES)

    list(FILTER _nf_ot_includes
        EXCLUDE REGEX "msys64/ucrt64/include")

    set_property(TARGET nexusflow_opentelemetry
        PROPERTY INCLUDE_DIRECTORIES "${_nf_ot_includes}")
endif()




