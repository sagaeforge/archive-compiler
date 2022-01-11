cmake_minimum_required(VERSION 3.0.0)
project(ProcessEvent VERSION 0.1.0)

include(CTest)
enable_testing()

add_library(
  ProcessEvent
  Sources/ProcessEvent.c
  Sources/Awake.c
  Sources/Init.c
  Sources/Start.c
  Sources/Main.c
  Sources/Update.c
  Sources/FixedUpdate.c
  Sources/Quit.c
)

target_include_directories(
  ProcessEvent
  PUBLIC

  # 모듈들
  ../Exception/Includes/Public
  ../GarbageCollection/Includes/Public
  ../InputSystem/Includes/Public
  ../ProcessEvent/Includes/Public

  # 상위 프로젝트의 헤더 참조
  ../../Includes/

  # String 모듈 참조
  ../../../String/Includes/Public
  
  Includes/Private
)

target_link_libraries(
  ProcessEvent
  pthread
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)