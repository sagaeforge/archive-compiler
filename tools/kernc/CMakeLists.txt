add_executable(kernc main.cpp)
target_link_libraries(kernc
    kern_codegen
    kern_ir
    kern_sema
    kern_parser
    kern_lexer
    kern_support
)
