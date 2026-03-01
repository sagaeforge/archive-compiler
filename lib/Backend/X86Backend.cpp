#include "kern/backend/X86Backend.h"
#include "kern/backend/Emitter.h"
#include "kern/backend/InstructionSelector.h"
#include "kern/backend/RegisterAllocator.h"

namespace kern {

MachModule* X86Backend::lower(const LIRModule& lir) {
    InstructionSelector isel(ctx_);
    return isel.select(lir);
}

void X86Backend::allocateRegisters(MachModule& mod) {
    RegisterAllocator ra(ctx_);
    for (uint32_t i = 0; i < mod.fn_count; ++i) {
        ra.run(mod.functions[i]);
    }
}

void X86Backend::emit(const LIRModule& lir, std::ostream& out) {
    auto* mach = lower(lir);
    allocateRegisters(*mach);

    NASMEmitter emitter(out);
    emitter.emitModule(*mach, lir);
}

} // namespace kern
