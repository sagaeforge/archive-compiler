cmake_minimum_required(VERSION 3.0.0)
project(String VERSION 0.1.0)

include(CTest)
enable_testing()

add_library(
  String

  Sources/String.c
)

target_include_directories(
  String
  PUBLIC

  # ProgramManager
  ../ProgramManager/Includes/Public/
  ../ProgramManager/Includes/Private/

  # String
  Includes/Public/
  Includes/Private/
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
