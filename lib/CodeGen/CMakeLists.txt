add_library(kern_codegen
    CodeGen.cpp
)
target_link_libraries(kern_codegen PUBLIC kern_ir kern_support)
