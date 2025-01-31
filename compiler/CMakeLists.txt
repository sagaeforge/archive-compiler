cmake_minimum_required(VERSION 3.25)
project(nugdev_compiler)

add_executable(
    ${PROJECT_NAME} 
    

    src/00_app/main.cpp
    src/00_app/repl/repl.cpp
    src/00_app/repl/repl.h
    src/00_app/lib/DontCopyable.h
    src/00_app/lib/PointerHelper.hpp
    src/00_app/lib/Singleton.h
)

target_include_directories(
    ${PROJECT_NAME}
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    ${PROJECT_NAME}
PRIVATE
    ICU::uc
    ICU::i18n
    ICU::data
)
