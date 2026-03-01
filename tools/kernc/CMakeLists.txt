add_executable(kernc main.cpp)
target_link_libraries(kernc
    kern_backend
    kern_lir
    kern_hir
    kern_codegen
    kern_ir
    kern_sema
    kern_parser
    kern_lexer
    kern_support
)
