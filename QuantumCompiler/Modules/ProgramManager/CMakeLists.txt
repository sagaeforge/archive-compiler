cmake_minimum_required(VERSION 3.0.0)
project(Exception VERSION 0.1.0)

include(CTest)
enable_testing()

add_subdirectory(Module/Exception)
add_subdirectory(Module/GarbageCollection)
add_subdirectory(Module/InputSystem)
add_subdirectory(Module/ProcessEvent)
add_subdirectory(Module/Object)

add_library(
  ProgramManager

  Sources/ProgramManager.c

)

target_include_directories(
  ProgramManager
  PUBLIC

  Includes/

  Exception/Includes/Public
  GarbageCollection/Includes/Public
  InputSystem/Includes/Public
  ProcessEvent/Includes/Public
  Object/Includes/Public

  # String
  ../String/Includes/Public
)
target_link_libraries(
  ProgramManager

  Exception
  GarbageCollection
  InputSystem
  ProcessEvent
  Object
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)