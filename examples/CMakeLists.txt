cmake_minimum_required(VERSION 3.14)
project(cppbot-example LANGUAGES CXX)
include("../cmake/utils.cmake")

# Change this value to a file you want to make
set(example message_sending.cpp)

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}"
    is_top_level)

if(is_top_level)
    find_package(cppbot REQUIRED)
endif()

set(sources ${example})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${sources})

add_executable(cppbot-example)
target_sources(cppbot-example PRIVATE ${sources})
target_link_libraries(cppbot-example PRIVATE cppbot)

if(NOT is_top_level)
    win_copy_deps_to_target_dir(cppbot-example cppbot)
endif()
