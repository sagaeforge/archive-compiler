add_library(kern_ir
    IRBuilder.cpp
)
target_link_libraries(kern_ir PUBLIC kern_parser kern_support)
