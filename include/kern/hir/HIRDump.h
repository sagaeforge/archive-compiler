#pragma once
#include "kern/hir/HIR.h"
#include <ostream>

namespace kern {

class TypeTable;

void dumpHIR(const HIRModule* mod, const TypeTable& types, std::ostream& out);
void dumpHIRExpr(const HIRExpr* expr, const TypeTable& types, std::ostream& out, int indent = 0);
void dumpHIRStmt(const HIRStmt* stmt, const TypeTable& types, std::ostream& out, int indent = 0);

} // namespace kern
