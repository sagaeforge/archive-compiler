cmake_minimum_required(VERSION 3.25)
project(nugdev_compiler)

add_library(
    ${PROJECT_NAME}_lib
    
    src/00_app/repl/repl.cpp
    src/00_app/repl/repl.h
    src/00_app/lib/DontCopyable.h
    src/00_app/lib/PointerHelper.hpp
    src/00_app/lib/Singleton.h
    src/00_app/stream/Stream.hpp

    src/01_tokenize/Tokenizer.h
    src/01_tokenize/Tokenizer.cpp
    src/01_tokenize/Token.h
    src/01_tokenize/Token.cpp
    src/01_tokenize/TokenFactory.h
    src/01_tokenize/factory/IdentifierTokenFactory.h
    src/01_tokenize/factory/IdentifierTokenFactory.cpp
    src/01_tokenize/factory/OperatorTokenFactory.h
    src/01_tokenize/factory/OperatorTokenFactory.cpp
    src/01_tokenize/factory/KeywordTokenFactory.h
    src/01_tokenize/factory/KeywordTokenFactory.cpp
    src/01_tokenize/factory/NumberTokenFactory.h
    src/01_tokenize/factory/NumberTokenFactory.cpp
    src/01_tokenize/factory/StringTokenFactory.h
    src/01_tokenize/factory/StringTokenFactory.cpp

    src/02_parsing/AST.h
    src/02_parsing/ASTAspect.h
    
)

add_executable(
    ${PROJECT_NAME}_exec
    
    src/00_app/main.cpp
)

add_executable(
    ${PROJECT_NAME}_test

    test/test.cpp
    test/01_tokenize/identifier_test.cpp
    test/01_tokenize/operator_test.cpp
    test/01_tokenize/keyword_test.cpp
    test/01_tokenize/string_test.cpp
    test/01_tokenize/number_test.cpp
)

target_include_directories(
    ${PROJECT_NAME}_lib
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_include_directories(
    ${PROJECT_NAME}_exec
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_include_directories(
    ${PROJECT_NAME}_test
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GTEST_INCLUDE_DIRS}
)

target_link_libraries(
    ${PROJECT_NAME}_lib
PUBLIC
    ICU::uc
    ICU::i18n
    ICU::data
    rapidjson
)

target_link_libraries(
    ${PROJECT_NAME}_test
PRIVATE
    ${PROJECT_NAME}_lib
    GTest::gtest_main
)

target_link_libraries(
    ${PROJECT_NAME}_exec
PRIVATE
    ${PROJECT_NAME}_lib
)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_test)