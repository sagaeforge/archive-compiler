#pragma once
#include "kern/debug/DebugInfo.h"
#include <cstdint>
#include <string_view>
#include <vector>

namespace kern {

// Builds source mappings during code emission.
// Each MachIR instruction can carry a SourceLocation; SourceMapBuilder
// records (address_offset, location) pairs as the emitter writes bytes.

class SourceMapBuilder {
public:
    // Record a mapping: the instruction at byte offset `addr` corresponds
    // to source location (line, column, file).
    void addMapping(uint64_t addr, uint32_t line, uint32_t column,
                    std::string_view file);

    // Finalize: sort by address and deduplicate consecutive identical locations.
    std::vector<SourceMapping> build();

    // Reset for reuse.
    void clear();

    size_t size() const { return mappings_.size(); }

private:
    std::vector<SourceMapping> mappings_;
};

} // namespace kern
