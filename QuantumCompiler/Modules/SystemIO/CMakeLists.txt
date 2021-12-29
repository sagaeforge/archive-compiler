cmake_minimum_required(VERSION 3.0.0)
project(SystemIO VERSION 0.1.0)

include(CTest)
enable_testing()

add_library(
  SystemIO

  Sources/SystemIO.c
)

target_include_directories(
  SystemIO
  PUBLIC

  # ProgramManager
  ../ProgramManager/Includes/Public/

  # String
  ../String/Includes/Public/

  # SystemIO
  Includes/Public/
  Includes/Private/

  
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
