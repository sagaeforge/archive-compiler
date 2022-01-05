cmake_minimum_required(VERSION 3.0.0)
project(GarbageCollection VERSION 0.1.0)

include(CTest)
enable_testing()

add_library(
  GarbageCollection
  Sources/GarbageCollection.c
  Sources/GetPage.c
  Sources/Info.c
  Sources/Policy.c
  Sources/Compare.c
  Sources/Copy.c
  Sources/Create.c
  Sources/Length.c
  Sources/Move.c
  Sources/Remove.c
  Sources/Set.c
  Sources/Swap.c
  Sources/GC_Append.c
  Sources/GC_Remove.c
  Sources/ConstCreate.c
)

target_include_directories(
  GarbageCollection
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