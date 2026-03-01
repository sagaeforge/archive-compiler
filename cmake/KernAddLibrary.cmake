# KernAddLibrary.cmake — Shared macro for adding Kern libraries
#
# Usage:
#   kern_add_library(kern_hir
#       HIRBuilder.cpp
#       HIRDump.cpp
#       DEPENDS kern_support kern_parser
#   )
#
# Creates a static library with standard include dirs and optional dependencies.

function(kern_add_library target)
    cmake_parse_arguments(ARG "" "" "DEPENDS" ${ARGN})

    # Everything before DEPENDS is a source file
    set(sources ${ARG_UNPARSED_ARGUMENTS})

    add_library(${target} ${sources})
    target_include_directories(${target} PUBLIC ${CMAKE_SOURCE_DIR}/include)

    if(ARG_DEPENDS)
        target_link_libraries(${target} PUBLIC ${ARG_DEPENDS})
    endif()

    # Label for ctest filtering: kern_test -L support
    string(REGEX REPLACE "^kern_" "" label ${target})
    set_target_properties(${target} PROPERTIES LABELS "${label}")
endfunction()
