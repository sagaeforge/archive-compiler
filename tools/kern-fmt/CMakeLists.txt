add_executable(kern-fmt main.cpp)
target_link_libraries(kern-fmt PRIVATE kern_fmt kern_parser kern_lexer kern_support)
