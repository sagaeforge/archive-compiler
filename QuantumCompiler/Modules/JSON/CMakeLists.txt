cmake_minimum_required(VERSION 3.0.0)
project(JSON VERSION 0.1.0)

include(CTest)
enable_testing()

add_library(
  JSON
  Sources/Json.c
)

target_include_directories(
  JSON
  PUBLIC

  ../Application/Includes/Public/
  ../String/Includes/Public/

  Includes/Public/
  Includes/Private/
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
