cmake_minimum_required(VERSION 3.0.0)
project(ProgramManager VERSION 0.1.0)

include(CTest)
enable_testing()

add_library(
  ProgramManager
  Sources/Delegate.c
  Sources/ProgramManager.c
  Sources/GarbageCollection.c
  
  # Sources/Memory
  Sources/Memory/MemoryCompare.c
  Sources/Memory/MemoryCopy.c
  Sources/Memory/MemoryCreate.c
  Sources/Memory/MemoryLength.c
  Sources/Memory/MemoryMove.c
  Sources/Memory/MemoryRemove.c
  Sources/Memory/MemorySet.c 
  Sources/Memory/MemorySwap.c
)

target_include_directories(ProgramManager PUBLIC Includes/Public/ Includes/Private/)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
