#pragma once
#include "kern/backend/MachIR.h"
#include "kern/lir/LIR.h"
#include "kern/support/CompilationContext.h"
#include <ostream>

namespace kern {

// X86Backend orchestrates the full LIR → MachIR → NASM pipeline:
//   1. InstructionSelector: LIR → MachIR (virtual registers)
//   2. RegisterAllocator:   MachIR vregs → physical registers
//   3. NASMEmitter:         MachIR → NASM assembly text
class X86Backend {
    CompilationContext& ctx_;

public:
    explicit X86Backend(CompilationContext& ctx) : ctx_(ctx) {}

    // Full pipeline: LIR → MachIR → allocate → NASM
    void emit(const LIRModule& lir, std::ostream& out);

    // Partial: LIR → MachIR (for dump/debug)
    MachModule* lower(const LIRModule& lir);

    // Partial: run register allocation on MachModule
    void allocateRegisters(MachModule& mod);
};

} // namespace kern
