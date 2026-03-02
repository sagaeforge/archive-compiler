#pragma once
#include "kern/backend/MachIR.h"
#include "kern/lir/LIR.h"
#include <ostream>
#include <string_view>

namespace kern {

struct CompilationContext;

// Target architecture identifiers
enum class TargetArch : uint8_t {
    X86_64,
    AArch64,
};

// Abstract interface for target-specific backends.
// Each architecture implements this to handle LIR → assembly.
class TargetBackend {
public:
    virtual ~TargetBackend() = default;

    virtual TargetArch arch() const = 0;
    virtual std::string_view archName() const = 0;

    // Full pipeline: LIR → MachIR → register alloc → assembly
    virtual void emit(const LIRModule& lir, std::ostream& out) = 0;

    // Partial: LIR → MachIR (for dump/debug)
    virtual MachModule* lower(const LIRModule& lir) = 0;

    // Partial: register allocation on MachModule
    virtual void allocateRegisters(MachModule& mod) = 0;
};

// Factory: create a backend for the given target
TargetBackend* createBackend(TargetArch arch, CompilationContext& ctx);

} // namespace kern
