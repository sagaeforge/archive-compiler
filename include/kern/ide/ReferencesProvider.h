#pragma once
#include "kern/support/SourceLocation.h"
#include <cstdint>
#include <string_view>
#include <vector>

namespace kern {

class IDEContext;

struct ReferenceLocation {
    SourceLocation location;
    bool is_definition;
};

// Finds all references to a symbol at a given position.

class ReferencesProvider {
public:
    std::vector<ReferenceLocation> findReferences(
        IDEContext& ctx, std::string_view path,
        uint32_t line, uint32_t column,
        bool include_definition = true);
};

} // namespace kern
