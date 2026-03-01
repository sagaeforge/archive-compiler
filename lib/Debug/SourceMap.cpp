#include "kern/debug/SourceMap.h"
#include <algorithm>

namespace kern {

void SourceMapBuilder::addMapping(uint64_t addr, uint32_t line,
                                  uint32_t column, std::string_view file) {
    mappings_.push_back({addr, line, column, file});
}

std::vector<SourceMapping> SourceMapBuilder::build() {
    // Sort by address
    std::sort(mappings_.begin(), mappings_.end(),
              [](const SourceMapping& a, const SourceMapping& b) {
                  return a.addr < b.addr;
              });

    // Deduplicate: remove consecutive entries with same line+col+file
    std::vector<SourceMapping> result;
    for (const auto& m : mappings_) {
        if (!result.empty() &&
            result.back().line == m.line &&
            result.back().column == m.column &&
            result.back().file == m.file) {
            continue;
        }
        result.push_back(m);
    }
    return result;
}

void SourceMapBuilder::clear() {
    mappings_.clear();
}

} // namespace kern
