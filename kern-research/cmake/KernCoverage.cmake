# KernCoverage.cmake — Coverage build support
#
# Usage in root CMakeLists.txt:
#   option(KERN_COVERAGE "Enable code coverage" OFF)
#   if(KERN_COVERAGE)
#       include(cmake/KernCoverage.cmake)
#       kern_enable_coverage()
#   endif()
#
# Then build:
#   cmake -B build-cov -DKERN_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
#   cmake --build build-cov
#   build-cov/tests/unit/kern_tests
#   bash scripts/run_coverage.sh build-cov

function(kern_enable_coverage)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
        message(FATAL_ERROR "Coverage requires Clang (found ${CMAKE_CXX_COMPILER_ID})")
    endif()

    add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
    add_link_options(-fprofile-instr-generate -fcoverage-mapping)

    message(STATUS "Coverage enabled: -fprofile-instr-generate -fcoverage-mapping")
endfunction()
