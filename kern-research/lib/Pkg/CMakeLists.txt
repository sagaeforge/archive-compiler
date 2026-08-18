add_library(kern_pkg
    Manifest.cpp
)
target_include_directories(kern_pkg PUBLIC ${CMAKE_SOURCE_DIR}/include)
