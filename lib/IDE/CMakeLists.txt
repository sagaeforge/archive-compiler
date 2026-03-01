add_library(kern_ide
    IDEContext.cpp
    CompletionProvider.cpp
    HoverProvider.cpp
    DefinitionProvider.cpp
    ReferencesProvider.cpp
    SemanticTokens.cpp
    DiagnosticProvider.cpp
)
target_include_directories(kern_ide PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(kern_ide PUBLIC kern_hir kern_parser kern_lexer kern_support)
