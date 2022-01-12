cmake_minimum_required(VERSION 3.0.0)
project(Object VERSION 0.1.0)

include(CTest)
enable_testing()

add_library(
  Object

  Sources/Object.c
)

target_include_directories(
  Object
  PUBLIC

  # 모듈들
  ../Exception/Includes/Public
  ../GarbageCollection/Includes/Public
  ../InputSystem/Includes/Public
  ../ProcessEvent/Includes/Public
  ../Object/Includes/Public

  # 상위 프로젝트의 헤더 참조
  ../../Includes/

  # String 헤더 참조
  ../../../String/Includes/Public

  Includes/Private
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
