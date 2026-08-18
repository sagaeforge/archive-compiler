# KernAddTool.cmake — Shared macro for adding Kern tool binaries
#
# Usage:
#   kern_add_tool(kernc
#       main.cpp
#       DEPENDS kern_pipeline kern_codegen
#   )
#
# Creates an executable in tools/<name>/ with standard include dirs and dependencies.

function(kern_add_tool target)
    cmake_parse_arguments(ARG "" "" "DEPENDS" ${ARGN})

    set(sources ${ARG_UNPARSED_ARGUMENTS})

    add_executable(${target} ${sources})
    target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/include)

    if(ARG_DEPENDS)
        target_link_libraries(${target} PRIVATE ${ARG_DEPENDS})
    endif()
endfunction()
