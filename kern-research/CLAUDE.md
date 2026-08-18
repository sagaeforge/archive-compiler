# Kern Compiler

Pure functional language compiler for x86-64 macOS kernel development.
C++20, CMake 3.20+, NASM + ld. Machine is arm64 (Rosetta 2).

## Build & Test (모든 코드 변경 후 반드시 실행)

```bash
cmake -B build && cmake --build build
build/tests/unit/kern_tests
bash tests/integration/run_tests.sh build/tools/kernc/kernc tests/integration
```

## Pipeline (4-level IR)

```
Source → Lexer → Parser → AST → HIRBuilder → HIR → LIRBuilder → LIR → Backend → MachIR → NASM → ld
```

Each stage: `include/kern/<stage>/` headers, `lib/<Stage>/` implementation.

## Directory Structure

```
include/kern/{lexer,parser,support}/         — core headers
include/kern/{hir,lir,backend}/              — IR pipeline headers
include/kern/{ide,fmt,debug,pipeline,pkg}/   — tooling headers
lib/{Lexer,Parser,Support}/                  — core implementations
lib/{HIR,LIR,Backend}/                       — IR pipeline implementations
lib/{IDE,Fmt,Debug,Pipeline,Pkg}/            — tooling implementations
tools/kernc/main.cpp                         — compiler driver
tests/unit/                                  — GoogleTest unit tests
tests/integration/                           — .kern + .expected E2E tests
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
