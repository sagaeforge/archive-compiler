#pragma once
#include "kern/hir/HIRPass.h"

namespace kern {

// Analyzes function purity: Pure, ImpureMut (var), ImpureMem (ptr write), ImpureIo (intrinsic).
// Sets HIRFnDecl::purity for every function.
// Propagation: ImpureMut is local-only; ImpureMem and ImpureIo propagate to callers.
class PurityAnalysisPass : public HIRPass {
public:
    std::string_view name() const override { return "PurityAnalysis"; }
    void run(HIRModule& module, CompilationContext& ctx) override;
};

// Marks HIRCallExpr::is_tail_call for calls in tail position.
// Sets HIRFnDecl::is_recursive and is_tail_recursive.
class TailCallAnalysisPass : public HIRPass {
public:
    std::string_view name() const override { return "TailCallAnalysis"; }
    void run(HIRModule& module, CompilationContext& ctx) override;
};

} // namespace kern
