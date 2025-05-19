add_library(compiler_lib SHARED
    main.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/01_lib/String.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/01_lib/String.h
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
