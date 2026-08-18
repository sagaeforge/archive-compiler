#pragma once
#include "kern/backend/MachIR.h"
#include <ostream>

namespace kern {

struct LIRModule;  // forward decl for globals

void dumpMachIR(const MachModule* mod, const LIRModule* lir_mod,
                const TypeTable& types, std::ostream& out);
void dumpMachFunction(const MachFunction& fn, const TypeTable& types,
                      std::ostream& out);
void dumpMachInstr(const MachInstr& instr, std::ostream& out);

} // namespace kern
