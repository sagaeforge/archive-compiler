cmake_minimum_required(VERSION 3.0.0)
project(QuantumCompiler VERSION 0.1.0)

include(CTest)
enable_testing()
set ( CMAKE_VERBOSE_MAKEFILE true )

add_executable(
  QuantumCompiler
  QuantumCompiler.c
)

add_subdirectory(Modules/ProgramManager/)
target_link_libraries(
  QuantumCompiler
  PUBLIC
  
  ProgramManager
)
target_include_directories(
  QuantumCompiler
  PUBLIC

  # QuantumCompiler
  Includes/

  # ProgramManager
  Modules/ProgramManager/Includes/Public/
  Modules/ProgramManager/Includes/Private/

)
target_compile_options(
  QuantumCompiler
  PUBLIC
  
  -Wall
)

add_definitions( 
  -DDEBUG
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
