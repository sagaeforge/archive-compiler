add_library(compiler_lib SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/01_lib/String.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/01_lib/String.h
    ${CMAKE_CURRENT_SOURCE_DIR}/01_lib/Char.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/01_lib/Char.h

    ${CMAKE_CURRENT_SOURCE_DIR}/02_scanning/Scanner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/02_scanning/Scanner.h

    ${CMAKE_CURRENT_SOURCE_DIR}/03_tokenize/Token.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/03_tokenize/Token.h
    ${CMAKE_CURRENT_SOURCE_DIR}/03_tokenize/TokenType.h
    ${CMAKE_CURRENT_SOURCE_DIR}/03_tokenize/TokenConverter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/03_tokenize/TokenConverter.h
)

target_include_directories(compiler_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(compiler_lib
PUBLIC
    ICU::uc
    ICU::i18n
    ICU::data
    RapidJSON rapidjson
    magic_enum::magic_enum
    Boost::uuid
)
target_include_directories(compiler_lib
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
