cmake_minimum_required(VERSION 3.0.0)
project(bbi VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(
    bbi
    bbi_code.cpp
    bbi.h
    bbi_pars.cpp
    bbi_tbl.cpp
    bb_add.bbi
    bbi.cpp
    bbi_misc.cpp
    bbi_prot.h
    bbi_tkn.cpp
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
