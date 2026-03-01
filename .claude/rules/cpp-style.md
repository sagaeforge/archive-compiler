---
paths:
  - "**/*.cpp"
  - "**/*.h"
---
# C++ Coding Conventions

- C++20 standard, -Wall -Wextra -Wpedantic -Werror
- #pragma once (not include guards)
- namespace kern { ... } // namespace kern
- Indent: 4 spaces, no tabs. K&R braces (opening on same line).
- PascalCase: classes, structs, enums (CodeGen, IRType, Purity::Pure)
- camelCase: methods (emitModule, typeOfExpr, regForWidth)
- snake_case_: private members with trailing underscore (out_, diag_, has_errors_)
- snake_case: locals and parameters (kern_file, test_name)
- ALL_CAPS: constants and static arrays (ARG_REGS, MAX_ARG_REGS)
- Arena allocation for AST/IR nodes — never raw new for these
- Minimal includes: own header first, then stdlib
- static for file-local helper functions
