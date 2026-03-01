#include "kern/ide/DiagnosticProvider.h"
#include "kern/ide/IDEContext.h"

namespace kern {

std::vector<IDEDiagnostic> DiagnosticProvider::diagnose(
    IDEContext& ctx, std::string_view path) {
    std::vector<IDEDiagnostic> result;

    if (!ctx.hasFile(path)) return result;

    // Trigger lazy rebuild (parse + HIR build), which populates diagnostics
    ctx.getHIR(path);

    // Collect diagnostics from the compilation context
    for (const auto& diag : ctx.context().diag.diagnostics()) {
        result.push_back({diag.level, diag.loc, diag.message});
    }

    return result;
}

} // namespace kern
