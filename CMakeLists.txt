cmake_minimum_required(VERSION 3.5)

project(tvterm)

# Target 'tvterm'

file(GLOB_RECURSE TVTERM_SRC
    "${CMAKE_CURRENT_LIST_DIR}/src/*.cc"
)

add_executable(${PROJECT_NAME} ${TVTERM_SRC})

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/include"
)

# Dependencies

find_library(VTERM vterm)
find_library(UTIL util)
add_subdirectory(deps/tvision)

target_link_libraries(${PROJECT_NAME} tvision ${VTERM} ${UTIL})

# Build optimization

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16.0")
    set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD ON)
    target_precompile_headers(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/include/pch.h"
    )
endif()
