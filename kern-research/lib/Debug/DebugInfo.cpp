#include "kern/debug/DebugInfo.h"
#include <algorithm>

namespace kern {

const SourceMapping* DebugInfo::findMapping(uint64_t addr) const {
    if (source_map.empty()) return nullptr;

    // Binary search: find last mapping with addr <= target
    auto it = std::upper_bound(
        source_map.begin(), source_map.end(), addr,
        [](uint64_t a, const SourceMapping& m) { return a < m.addr; });

    if (it == source_map.begin()) return nullptr;
    --it;
    return &(*it);
}

const FunctionDebugInfo* DebugInfo::findFunction(uint64_t addr) const {
    for (const auto& fn : functions) {
        if (addr >= fn.code_start && addr < fn.code_end) {
            return &fn;
        }
    }
    return nullptr;
}

const LocalVarInfo* DebugInfo::findLocal(const FunctionDebugInfo& fn,
                                         std::string_view name) const {
    for (const auto& var : fn.locals) {
        if (var.name == name) return &var;
    }
    return nullptr;
}

} // namespace kern
