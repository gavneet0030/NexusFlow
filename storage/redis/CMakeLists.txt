cmake_minimum_required(VERSION 3.20)

add_library(nexusflow_redis
    redis_store.cpp
)

target_include_directories(nexusflow_redis
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WIN32)
    target_include_directories(nexusflow_redis
        PRIVATE
            C:/vcpkg/installed/x64-windows/include
    )

    target_link_directories(nexusflow_redis
        PRIVATE
            C:/vcpkg/installed/x64-windows/lib
    )

    target_link_libraries(nexusflow_redis
        PRIVATE
            redis++
            hiredis
    )
else()
    find_path(REDIS_PLUS_PLUS_INCLUDE_DIR
        NAMES sw/redis++/redis++.h
        PATHS
            /usr/include
            /usr/local/include
    )

    find_path(HIREDIS_INCLUDE_DIR
        NAMES hiredis/hiredis.h
        PATHS
            /usr/include
            /usr/local/include
    )

    find_library(REDIS_PLUS_PLUS_LIBRARY
        NAMES redis++
        PATHS
            /usr/lib
            /usr/lib/x86_64-linux-gnu
            /usr/local/lib
    )

    find_library(HIREDIS_LIBRARY
        NAMES hiredis
        PATHS
            /usr/lib
            /usr/lib/x86_64-linux-gnu
            /usr/local/lib
    )

    if(NOT REDIS_PLUS_PLUS_INCLUDE_DIR)
        message(FATAL_ERROR
            "Redis++ headers not found. Install libredis++-dev."
        )
    endif()

    if(NOT HIREDIS_INCLUDE_DIR)
        message(FATAL_ERROR
            "Hiredis headers not found. Install libhiredis-dev."
        )
    endif()

    if(NOT REDIS_PLUS_PLUS_LIBRARY)
        message(FATAL_ERROR
            "Redis++ library not found."
        )
    endif()

    if(NOT HIREDIS_LIBRARY)
        message(FATAL_ERROR
            "Hiredis library not found."
        )
    endif()

    target_include_directories(nexusflow_redis
        PRIVATE
            ${REDIS_PLUS_PLUS_INCLUDE_DIR}
            ${HIREDIS_INCLUDE_DIR}
    )

    target_link_libraries(nexusflow_redis
        PRIVATE
            ${REDIS_PLUS_PLUS_LIBRARY}
            ${HIREDIS_LIBRARY}
    )
endif()
