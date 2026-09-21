cmake_minimum_required(VERSION 3.20)

project(RdKafkaProbe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(RdKafka CONFIG REQUIRED)

message(STATUS "RDKAFKA PACKAGE FOUND")

if(TARGET RdKafka::rdkafka)
    message(STATUS "RDKAFKA C TARGET FOUND")
else()
    message(FATAL_ERROR "RdKafka::rdkafka target not found")
endif()

add_executable(rdkafka_probe main.cpp)

target_link_libraries(
    rdkafka_probe
    PRIVATE
    RdKafka::rdkafka
)