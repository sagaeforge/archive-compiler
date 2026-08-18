add_executable(kern-lsp main.cpp)
target_link_libraries(kern-lsp PRIVATE kern_ide kern_hir kern_parser kern_lexer kern_support)
