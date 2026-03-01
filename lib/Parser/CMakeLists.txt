add_library(kern_parser
    Parser.cpp
)
target_link_libraries(kern_parser PUBLIC kern_lexer kern_support)
