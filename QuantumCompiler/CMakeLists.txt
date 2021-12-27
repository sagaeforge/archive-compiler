cmake_minimum_required(VERSION 3.0.0)
project(QuantumCompiler VERSION 0.1.0)

include(CTest)
enable_testing()
set ( CMAKE_VERBOSE_MAKEFILE true )

add_executable(
  QuantumCompiler
  QuantumCompiler.c

  Sources/Parser.c
)

add_subdirectory(Modules/ProgramManager/)
add_subdirectory(Modules/String/)
target_link_libraries(
  QuantumCompiler
  PUBLIC
  
  ProgramManager
  String
)
target_include_directories(
  QuantumCompiler
  PUBLIC

  # QuantumCompiler
  Includes/

  # ProgramManager
  Modules/ProgramManager/Includes/Public/
  Modules/ProgramManager/Includes/Private/

  # String
  Modules/String/Includes/Public/
  Modules/String/Includes/Private/
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
