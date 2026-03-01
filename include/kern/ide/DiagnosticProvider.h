#pragma once
#include "kern/support/Diagnostic.h"
#include <string_view>
#include <vector>

namespace kern {

class IDEContext;

struct IDEDiagnostic {
    DiagLevel severity;
    SourceLocation loc;
    std::string message;
};

class DiagnosticProvider {
public:
    std::vector<IDEDiagnostic> diagnose(IDEContext& ctx, std::string_view path);
};

} // namespace kern
