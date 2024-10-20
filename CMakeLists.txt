cmake_minimum_required(VERSION 3.5)

option(TVTERM_BUILD_APP "Build main application" ON)
option(TVTERM_USE_SYSTEM_TVISION "Use system-wide Turbo Vision instead of the submodule" OFF)
option(TVTERM_OPTIMIZE_BUILD "Enable build optimizations (Unity Build, Precompiled Headers)" ON)

project(tvterm)

function(tvterm_set_warnings t)
    if (NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC"))
        target_compile_options(${t} PRIVATE
            -Wall
        )
    endif()
endfunction()

# Dependencies

include(deps/CMakeLists.txt)

include(CheckCXXSourceCompiles)

set(CMAKE_REQUIRED_INCLUDES ${vterm_INCLUDE})
check_cxx_source_compiles("
    #include <vterm.h>

    int main() { VTermStringFragment str; }
"   HAVE_VTERMSTRINGFRAGMENT)
unset(CMAKE_REQUIRED_INCLUDES)

# Target 'tvterm-core'

file(GLOB_RECURSE TVTERM_CORE_SRC "${CMAKE_CURRENT_LIST_DIR}/source/tvterm-core/*.cc")
add_library(tvterm-core ${TVTERM_CORE_SRC})

tvterm_set_warnings(tvterm-core)
if (HAVE_VTERMSTRINGFRAGMENT)
    target_compile_definitions(tvterm-core PRIVATE HAVE_VTERMSTRINGFRAGMENT)
endif()
target_include_directories(tvterm-core PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
    "$<INSTALL_INTERFACE:include>"
)
target_include_directories(tvterm-core PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/source/tvterm-core"
)
target_link_libraries(tvterm-core PUBLIC
    tvision
    vterm
    util
    pthread
)
install(TARGETS vterm util pthread EXPORT tvterm-config)
install(TARGETS tvterm-core
    EXPORT tvterm-config
    ARCHIVE DESTINATION lib
    COMPONENT library
)
install(EXPORT tvterm-config
    DESTINATION lib/cmake/tvterm
    NAMESPACE tvterm::
    FILE tvterm-config.cmake
    COMPONENT library
)
install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/include/tvterm" DESTINATION include)

# Target 'tvterm'

if (TVTERM_BUILD_APP)
    file(GLOB_RECURSE TVTERM_SRC "${CMAKE_CURRENT_LIST_DIR}/source/tvterm/*.cc")
    add_executable(tvterm ${TVTERM_SRC})
    tvterm_set_warnings(tvterm)
    target_link_libraries(tvterm PRIVATE
        tvterm-core
    )
    install(TARGETS tvterm RUNTIME DESTINATION bin)
endif()

# Build optimization

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16.0")
    if (TVTERM_OPTIMIZE_BUILD)
        foreach (t tvterm tvterm-core)
            set_target_properties(${t} PROPERTIES UNITY_BUILD ON)
            target_precompile_headers(${t} PRIVATE
                "${CMAKE_CURRENT_LIST_DIR}/include/pch.h"
            )
        endforeach()
    endif()
endif()
