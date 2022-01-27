cmake_minimum_required(VERSION 3.0.0)
project(QuantumCompiler VERSION 0.1.0)

include(CTest)
enable_testing()
# 상세 과정 출력
set ( CMAKE_VERBOSE_MAKEFILE true )

add_executable(
  QuantumCompiler
  QuantumCompiler.c

  Sources/Parser.c
)

add_subdirectory(Modules/Application/)
add_subdirectory(Modules/String/)
add_subdirectory(Modules/JSON/)
target_link_libraries(
  QuantumCompiler
  PUBLIC
  
  Application
  String
  JSON
)
target_include_directories(
  QuantumCompiler
  PUBLIC

  # QuantumCompiler
  Includes/

  # ProgramManager
  Modules/Application/Includes/Public/

  # String
  Modules/String/Includes/Public/

  # JSON
  Modules/JSON/Includes/Public/
)
target_compile_options(
  QuantumCompiler
  PUBLIC
  
  -Wall
  -std=gnu17
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)