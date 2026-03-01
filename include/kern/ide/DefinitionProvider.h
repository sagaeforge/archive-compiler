#pragma once
#include "kern/support/SourceLocation.h"
#include <cstdint>
#include <optional>
#include <string_view>

namespace kern {

class IDEContext;

struct DefinitionResult {
    SourceLocation location;
    std::string_view name;
};

// Finds the definition site of a symbol at a given position.

class DefinitionProvider {
public:
    std::optional<DefinitionResult> findDefinition(
        IDEContext& ctx, std::string_view path,
        uint32_t line, uint32_t column);
};

} // namespace kern
