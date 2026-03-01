#pragma once
#include "kern/debug/DebugInfo.h"
#include "kern/support/CompilationContext.h"
#include <ostream>
#include <istream>

namespace kern {

struct MachFunction;
struct MachModule;

// Builds DebugInfo from MachIR + CompilationContext.
// Activated by the -g compiler flag.

class DebugInfoBuilder {
public:
    explicit DebugInfoBuilder(CompilationContext& ctx);

    // Build debug info from a MachModule.
    DebugInfo build(const MachModule& mod);

    // Serialize debug info to a binary stream (.kern_debug format).
    static void serialize(const DebugInfo& info, std::ostream& out);

    // Deserialize debug info from a binary stream.
    static DebugInfo deserialize(std::istream& in);

private:
    CompilationContext& ctx_;

    FunctionDebugInfo buildFunction(const MachFunction& fn);
};

} // namespace kern
