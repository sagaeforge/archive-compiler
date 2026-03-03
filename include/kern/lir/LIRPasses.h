#pragma once
#include "kern/lir/LIRPass.h"

namespace kern {

// Fold constant arithmetic: ConstInt + ConstInt → ConstInt, etc.
class ConstFoldPass : public LIRPass {
public:
    std::string_view name() const override { return "const-fold"; }
    void run(LIRModule& module, CompilationContext& ctx) override;
};

// Remove instructions whose results are never used.
class DeadCodeElimPass : public LIRPass {
public:
    std::string_view name() const override { return "dce"; }
    void run(LIRModule& module, CompilationContext& ctx) override;
};

// Propagate known constants through vregs to enable more folding.
class ConstPropPass : public LIRPass {
public:
    std::string_view name() const override { return "const-prop"; }
    void run(LIRModule& module, CompilationContext& ctx) override;
};

// Local common subexpression elimination: within a basic block,
// if the same pure computation appears twice, reuse the first result.
class CSEPass : public LIRPass {
public:
    std::string_view name() const override { return "cse"; }
    void run(LIRModule& module, CompilationContext& ctx) override;
};

// Inline small functions at their call sites.
class InliningPass : public LIRPass {
public:
    std::string_view name() const override { return "inline"; }
    void run(LIRModule& module, CompilationContext& ctx) override;
};

} // namespace kern
