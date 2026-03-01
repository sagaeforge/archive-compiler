# Kern Compiler

Pure functional language compiler for x86-64 macOS kernel development.
C++20, CMake 3.20+, NASM + ld. Machine is arm64 (Rosetta 2).

## Build & Test (모든 코드 변경 후 반드시 실행)

```bash
cmake -B build && cmake --build build
build/tests/unit/kern_tests
bash tests/integration/run_tests.sh build/tools/kernc/kernc tests/integration
```

## Pipeline

v1 (legacy, still used for final binary emission):
```
Lexer → Parser → TypeChecker → PurityChecker → IRBuilder → CodeGen → NASM → ld
```

v2 (4-level IR, dump flags operational):
```
Source → Lexer → Parser → AST → HIRBuilder → HIR → LIRBuilder → LIR → Backend → MachIR → NASM → ld
```

Each stage: `include/kern/<stage>/` headers, `lib/<Stage>/` implementation.

## Directory Structure

```
include/kern/{lexer,parser,sema,ir,codegen,support}/  — v1 headers
include/kern/{hir,lir,backend}/                        — v2 headers
lib/{Lexer,Parser,Sema,IR,CodeGen,Support}/            — v1 implementations
lib/{HIR,LIR,Backend}/                                 — v2 implementations
tools/kernc/main.cpp                                    — compiler driver
tests/unit/                                             — GoogleTest unit tests
tests/integration/                                      — .kern + .expected E2E tests
```

## Agent Scope

- MAY modify: lib/, include/, tests/, tools/, CMakeLists.txt, .github/
- MAY NOT modify: docs/, README.md, REQUIREMENTS.md
- MAY NOT delete existing tests or change ABI without explicit approval

## Commit Format

```
feat|fix|test|refactor: <description>
```

## Rules

Coding conventions: @.claude/rules/cpp-style.md
Architecture invariants: @.claude/rules/architecture-v2.md
Layer boundaries: @.claude/rules/layer-boundaries.md
