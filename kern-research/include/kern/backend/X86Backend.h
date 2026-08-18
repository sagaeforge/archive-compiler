#pragma once
#include "kern/backend/TargetBackend.h"
#include "kern/backend/MachIR.h"
#include "kern/lir/LIR.h"
#include "kern/support/CompilationContext.h"
#include <ostream>

namespace kern {

// X86Backend orchestrates the full LIR → MachIR → NASM pipeline:
//   1. InstructionSelector: LIR → MachIR (virtual registers)
//   2. RegisterAllocator:   MachIR vregs → physical registers
//   3. NASMEmitter:         MachIR → NASM assembly text
class X86Backend : public TargetBackend {
    CompilationContext& ctx_;
    OutputFormat format_;

public:
    explicit X86Backend(CompilationContext& ctx,
                         OutputFormat fmt = OutputFormat::Macho64)
        : ctx_(ctx), format_(fmt) {}

    TargetArch arch() const override { return TargetArch::X86_64; }
    std::string_view archName() const override { return "x86-64"; }

    // Full pipeline: LIR → MachIR → allocate → NASM
    void emit(const LIRModule& lir, std::ostream& out) override;

    // Partial: LIR → MachIR (for dump/debug)
    MachModule* lower(const LIRModule& lir) override;

    // Partial: run register allocation on MachModule
    void allocateRegisters(MachModule& mod) override;
};

} // namespace kern
