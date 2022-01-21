cmake_minimum_required(VERSION 3.0.0)
project(Application VERSION 0.1.0)

include(CTest)
enable_testing()

add_library(
  Application

  Sources/Application.c
  Sources/ProcessEvent.c
  Sources/Object.c
  Sources/GarbageCollection.c
  Sources/DataTable.c
  Sources/DataType.c
  
  Sources/ProcessEvent/Awake.c
  Sources/ProcessEvent/Init.c
  Sources/ProcessEvent/Start.c
  Sources/ProcessEvent/Main.c
  Sources/ProcessEvent/Quit.c
  Sources/ProcessEvent/Update.c
  Sources/ProcessEvent/FixedUpdate.c

  # [*] GarbageCollect Start
  Sources/GarbageCollection/Object/GetObject.c
  Sources/GarbageCollection/Object/FreeObject.c
  
  Sources/GarbageCollection/Memory/MemoryCompare.c
  Sources/GarbageCollection/Memory/MemoryCopy.c
  Sources/GarbageCollection/Memory/MemoryLength.c
  Sources/GarbageCollection/Memory/MemoryDestructor.c
  Sources/GarbageCollection/Memory/MemorySwap.c
  Sources/GarbageCollection/Memory/MemoryConstructor.c
  Sources/GarbageCollection/Memory/MemoryCreate.c
  Sources/GarbageCollection/Memory/MemoryConstructor_FP.c
  Sources/GarbageCollection/Memory/MemoryConstructorAry.c
  Sources/GarbageCollection/Memory/MemoryMove.c
  Sources/GarbageCollection/Memory/MemorySet.c

  Sources/GarbageCollection/GarbageCollection_Append.c
  Sources/GarbageCollection/GarbageCollection_PageGet.c
  Sources/GarbageCollection/GarbageCollection_EmptyPageGet.c
  Sources/GarbageCollection/GarbageCollection_Remove.c
  Sources/GarbageCollection/GarbageCollection_Find.c
  # [!] GarbageCollect End

  Sources/Object/Object_Boxing_Int.c
  Sources/Object/Object_UnBoxing_Int.c

)

target_include_directories(
  Application
  PUBLIC

  Includes/Private
  Includes/Public
)

target_link_libraries(
  Application
  PUBLIC

  pthread
)

target_compile_options(
  Application
  PUBLIC
  
  # 특정 함수에서 값을 안사용하는 경우 컴파일 중지
  # -W
  -Wall -Wunused-result

  # 표준을 따를 것임을 알림
  -std=gnu17
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)