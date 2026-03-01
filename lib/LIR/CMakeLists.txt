add_library(kern_lir
    LIRBuilder.cpp
    LIRDump.cpp
)
target_include_directories(kern_lir PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(kern_lir PUBLIC kern_hir kern_support)
