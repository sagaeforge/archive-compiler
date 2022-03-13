cmake_minimum_required ( VERSION 3.0.0 )
project ( nCX_TEST VERSION 0.0.1 )

include ( CTest )
enable_testing ()

set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../build/${PROJECT_NAME}/${CMAKE_BUILD_TYPE}/${PROJECT_VERSION}/exec )
set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../build/${PROJECT_NAME}/${CMAKE_BUILD_TYPE}/${PROJECT_VERSION}/lib )
set ( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../build/${PROJECT_NAME}/${CMAKE_BUILD_TYPE}/${PROJECT_VERSION}/lib )

LINK_DIRECTORIES (
  ${CMAKE_SOURCE_DIR}/../build/nCX_Lib/${CMAKE_BUILD_TYPE}/${PROJECT_VERSION}/lib
)

add_executable ( nCX_TEST
  Main.c
)

target_link_libraries ( nCX_TEST
PUBLIC
  nCX_Lib
)

target_include_directories ( nCX_TEST
PUBLIC
  ${CMAKE_SOURCE_DIR}/../nCX_Library/Includes/nString/
  ${CMAKE_SOURCE_DIR}/../nCX_Library/Includes/nGarbageCollection/
)

set ( CPACK_PROJECT_NAME ${PROJECT_NAME} )
set ( CPACK_PROJECT_VERSION ${PROJECT_VERSION} )
include ( CPack )
