add_library(kern_fmt
    Formatter.cpp
)
target_include_directories(kern_fmt PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(kern_fmt PUBLIC kern_parser)
