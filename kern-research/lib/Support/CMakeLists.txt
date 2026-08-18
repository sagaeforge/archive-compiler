add_library(kern_support
    Arena.cpp
    BuildCache.cpp
    Diagnostic.cpp
    ModuleResolver.cpp
    StringPool.cpp
    TypeSystem.cpp
)
target_include_directories(kern_support PUBLIC ${CMAKE_SOURCE_DIR}/include)
