add_library(kern_hir
    HIRBuilder.cpp
    HIRDump.cpp
    HIRPasses.cpp
    MonomorphizationPass.cpp
)
target_include_directories(kern_hir PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(kern_hir PUBLIC kern_support)
