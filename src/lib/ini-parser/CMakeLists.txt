cmake_minimum_required ( VERSION 3.25 )
project (
  "nugdev_ini-parser"
  VERSION 0.1.0
  LANGUAGES CXX
)

set ( CMAKE_CXX_STANDARD 20 )
set ( CMAKE_CXX_STANDARD_REQUIRED ON )

add_library ( ${PROJECT_NAME} SHARED
        File.cpp
        File.h
        ContentType.h
        Content.h
        Comment.h
        Section.h
        Field.h
        Line.h
)
target_include_directories( ${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/.. )