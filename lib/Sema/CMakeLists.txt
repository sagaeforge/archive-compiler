add_library(kern_sema
    TypeChecker.cpp
    PurityChecker.cpp
)
target_link_libraries(kern_sema PUBLIC kern_parser kern_support)
