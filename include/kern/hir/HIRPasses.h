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

// Infers and enforces effect annotations on functions.
// Sets HIRFnDecl::inferred_effects using same algorithm as PurityAnalysisPass.
// Validates that declared_effects is a superset of inferred_effects.
// Enforces call-site effect checking: caller must have callee's effects.
class EffectAnalysisPass : public HIRPass {
public:
    std::string_view name() const override { return "EffectAnalysis"; }
    void run(HIRModule& module, CompilationContext& ctx) override;
};

// Ownership checking: detects use-after-move for 'own' parameters,
// borrow escape (returning/storing borrowed values), and
// simultaneous mutable borrow aliasing.
class OwnershipCheckPass : public HIRPass {
public:
    std::string_view name() const override { return "OwnershipCheck"; }
    void run(HIRModule& module, CompilationContext& ctx) override;
};

// Marks HIRCallExpr::is_tail_call for calls in tail position.
// Sets HIRFnDecl::is_recursive and is_tail_recursive.
class TailCallAnalysisPass : public HIRPass {
public:
    std::string_view name() const override { return "TailCallAnalysis"; }
    void run(HIRModule& module, CompilationContext& ctx) override;
};

// Detects constant integer overflow in binary operations with known operands.
// Errors when the result doesn't fit in the target type.
class ConstOverflowPass : public HIRPass {
public:
    std::string_view name() const override { return "ConstOverflow"; }
    void run(HIRModule& module, CompilationContext& ctx) override;
};

// Warns on narrowing casts that may lose data (unless explicit truncate/clamp).
class LossyCastPass : public HIRPass {
public:
    std::string_view name() const override { return "LossyCast"; }
    void run(HIRModule& module, CompilationContext& ctx) override;
};

// Detects returning references to local variables (borrow escape).
class BorrowEscapePass : public HIRPass {
public:
    std::string_view name() const override { return "BorrowEscape"; }
    void run(HIRModule& module, CompilationContext& ctx) override;
};

// Detects same variable passed as mutable borrow to multiple params.
class MutBorrowAliasPass : public HIRPass {
public:
    std::string_view name() const override { return "MutBorrowAlias"; }
    void run(HIRModule& module, CompilationContext& ctx) override;
};

// Warns on unused val/var bindings (names starting with _ are exempt).
class UnusedBindingPass : public HIRPass {
public:
    std::string_view name() const override { return "UnusedBinding"; }
    void run(HIRModule& module, CompilationContext& ctx) override;
};

class MustUseCheckPass : public HIRPass {
public:
    std::string_view name() const override { return "MustUseCheck"; }
    void run(HIRModule& module, CompilationContext& ctx) override;
};

} // namespace kern
