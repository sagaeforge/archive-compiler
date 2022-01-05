cmake_minimum_required(VERSION 3.0.0)
project(Exception VERSION 0.1.0)

include(CTest)
enable_testing()

add_library(
  Exception
  Sources/Exception.c
)

target_include_directories(
  Exception
  PUBLIC

  # 모듈들
  ../Exception/Includes/Public
  ../GarbageCollection/Includes/Public
  ../InputSystem/Includes/Public
  ../ProcessEvent/Includes/Public

  # 상위 프로젝트의 헤더 참조
  ../../Includes/

  Includes/Private
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)