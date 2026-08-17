add_library(compiler_lib SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/01_lib/String.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/01_lib/String.h
    ${CMAKE_CURRENT_SOURCE_DIR}/01_lib/Char.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/01_lib/Char.h

    ${CMAKE_CURRENT_SOURCE_DIR}/02_scanning/Scanner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/02_scanning/Scanner.h

    ${CMAKE_CURRENT_SOURCE_DIR}/03_tokenize/Token.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/03_tokenize/Token.h
    ${CMAKE_CURRENT_SOURCE_DIR}/03_tokenize/TokenType.h
    ${CMAKE_CURRENT_SOURCE_DIR}/03_tokenize/TokenConverter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/03_tokenize/TokenConverter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/03_tokenize/Tokenizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/03_tokenize/Tokenizer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/03_tokenize/TokenCategories.h

    # AST System - Core
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ast/core/ASTNode.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ast/core/ASTVisitor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ast/core/AST.hpp

    # AST System - Types
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ast/types/Types.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ast/types/Types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ast/types/Parameters.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ast/types/Parameters.cpp

    # AST System - Expressions
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ast/expressions/Expressions.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ast/expressions/Expressions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ast/expressions/ComplexExpressions.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ast/expressions/ComplexExpressions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ast/expressions/AdvancedExpressions.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ast/expressions/AdvancedExpressions.cpp

    # AST System - Literals
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ast/literals/Literals.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ast/literals/Literals.cpp

    # AST System - Control Flow & Statements
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ast/control_flow/ControlFlow.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ast/control_flow/ControlFlow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ast/control_flow/WhenConditions.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ast/control_flow/WhenConditions.cpp

    # AST System - Statement
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ast/statements/Statements.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ast/statements/Statements.cpp

    # AST System - Program Structure
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ast/program/Program.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ast/program/Program.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ast/program/ImportExport.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ast/program/ImportExport.cpp

    # AST System - Parser
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/Parser.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/Parser.cpp

    # Analysis Components - Core
    ${CMAKE_CURRENT_SOURCE_DIR}/05_analysis/AnalysisConfig.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/05_analysis/AnalysisConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/05_analysis/AnalysisManager.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/05_analysis/ImplementationTemplate.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/05_analysis/MissingASTSupport.hpp
)

target_include_directories(compiler_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(compiler_lib
PUBLIC
    ${CMAKE_THREAD_LIBS_INIT}
)
target_include_directories(compiler_lib
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
