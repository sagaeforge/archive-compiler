#pragma once
#include "kern/support/SourceLocation.h"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace kern {

class IDEContext;

struct HoverResult {
    std::string type_info;        // e.g. "fn(i64, i64) -> i64"
    std::string doc;              // future: doc comments
    std::string purity;           // "pure" / "impure(mut)" etc
    SourceLocation definition_loc;
};

// Returns hover information (type, docs) for an identifier at a position.

class HoverProvider {
public:
    std::optional<HoverResult> hover(
        IDEContext& ctx, std::string_view path,
        uint32_t line, uint32_t column);
};

} // namespace kern
