#pragma once
#include "kern/backend/MachIR.h"

namespace kern {

// Run peephole optimizations on a MachModule after register allocation.
// All vregs must be resolved to physical registers before calling this.
void peepholeOptimize(MachModule& mod);

} // namespace kern
