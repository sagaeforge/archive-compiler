add_executable(kern-repl main.cpp)
target_link_libraries(kern-repl PRIVATE kern_pipeline kern_backend kern_lir kern_hir kern_parser kern_lexer kern_support)
