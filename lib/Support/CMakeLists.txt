add_library(kern_support
    Arena.cpp
    Diagnostic.cpp
)
target_include_directories(kern_support PUBLIC ${CMAKE_SOURCE_DIR}/include)
