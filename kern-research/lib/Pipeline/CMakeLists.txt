add_library(kern_pipeline
    CompilerPipeline.cpp
)
target_link_libraries(kern_pipeline PUBLIC
    kern_backend
    kern_lir
    kern_hir
    kern_parser
    kern_lexer
    kern_support
)
