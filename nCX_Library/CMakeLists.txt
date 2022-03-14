cmake_minimum_required ( VERSION 3.0.0 )
project ( nCX_Lib VERSION 0.0.1 )

include ( CTest )
enable_testing ()

set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../build/${PROJECT_NAME}/${CMAKE_BUILD_TYPE}/${PROJECT_VERSION}/exec )
set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../build/${PROJECT_NAME}/${CMAKE_BUILD_TYPE}/${PROJECT_VERSION}/lib )
set ( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../build/${PROJECT_NAME}/${CMAKE_BUILD_TYPE}/${PROJECT_VERSION}/lib )

set ( nGarbageCollection_SRCS
  nGarbageCollection.c
)

set ( nString_SRCS
  nString.c
)

add_library ( nCX_Lib
  nCX_Library.c

  ${nGarbageCollection_SRCS}
  ${nString_SRCS}
)

target_include_directories ( nCX_Lib
PUBLIC
  Includes/
)

set ( CPACK_PROJECT_NAME ${PROJECT_NAME} )
set ( CPACK_PROJECT_VERSION ${PROJECT_VERSION} )
include ( CPack )
