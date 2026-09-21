add_library(nexusflow_postgres
    postgres_store.cpp
)

target_include_directories(nexusflow_postgres
    PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

if (WIN32)

    target_include_directories(nexusflow_postgres
        PRIVATE
            "C:/vcpkg/installed/x64-windows/include"
    )

    target_link_directories(nexusflow_postgres
        PRIVATE
            "C:/vcpkg/installed/x64-windows/lib"
    )

    target_link_libraries(nexusflow_postgres
        PUBLIC
            libpq
    )

else()

    find_path(POSTGRESQL_INCLUDE_DIR
        NAMES
            libpq-fe.h
        PATHS
            /usr/include
            /usr/include/postgresql
            /usr/local/include
    )

    find_library(POSTGRESQL_LIBRARY
        NAMES
            pq
            libpq
        PATHS
            /usr/lib
            /usr/lib/x86_64-linux-gnu
            /usr/local/lib
    )

    if (NOT POSTGRESQL_INCLUDE_DIR)
        message(FATAL_ERROR "PostgreSQL headers not found")
    endif()

    if (NOT POSTGRESQL_LIBRARY)
        message(FATAL_ERROR "PostgreSQL library not found")
    endif()

    target_include_directories(nexusflow_postgres
        PRIVATE
            ${POSTGRESQL_INCLUDE_DIR}
    )

    target_link_libraries(nexusflow_postgres
        PUBLIC
            ${POSTGRESQL_LIBRARY}
    )

    message(STATUS "LINUX POSTGRES INCLUDE: ${POSTGRESQL_INCLUDE_DIR}")
    message(STATUS "LINUX POSTGRES LIBRARY: ${POSTGRESQL_LIBRARY}")

endif()
