add_library(kern_debug
    DebugInfo.cpp
    SourceMap.cpp
    DebugInfoBuilder.cpp
    ValueInspector.cpp
)
target_include_directories(kern_debug PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(kern_debug PUBLIC kern_support)
