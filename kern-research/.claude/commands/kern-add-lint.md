# /kern:add-lint — Add Lint Rule Boilerplate

Generate boilerplate for a new lint rule.

## Arguments

$ARGUMENTS should be the rule name (e.g., `UnusedVariable`, `UncheckedResult`)

## Generated Files

1. `include/kern/lint/<Name>Lint.h` — LintPass subclass declaration
2. `lib/Lint/<Name>Lint.cpp` — Rule implementation
3. `tests/unit/lint/<Name>LintTest.cpp` — Positive + negative test cases

## Template

```cpp
// include/kern/lint/<Name>Lint.h
#pragma once
#include "kern/lint/LintPass.h"

namespace kern {

class <Name>Lint : public LintPass {
public:
    std::string_view name() const override { return "<Name>"; }
    void run(const HIRModule& module, DiagnosticEngine& diag) override;
};

} // namespace kern
```

## Post-Generation Steps

1. Add .cpp to `lib/Lint/CMakeLists.txt`
2. Add test to `tests/unit/CMakeLists.txt`
3. Register in LintEngine's default rule set
4. Run `/kern:build`

## Notes

- Each lint rule should have both positive (triggers warning) and negative (clean code) test cases
- Lint rules operate on HIR, not AST (access to type information)
