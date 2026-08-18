#pragma once
#include "kern/lir/LIR.h"
#include <ostream>

namespace kern {

void dumpLIR(const LIRModule* mod, const TypeTable& types, std::ostream& out);
void dumpLIRFunction(const LIRFunction* fn, const TypeTable& types, std::ostream& out);
void dumpLIRInstr(const LIRInstr& instr, const TypeTable& types, std::ostream& out);

} // namespace kern
