#pragma once
#include <cstdint>
#include <string_view>

namespace kern {

struct SourceLocation {
    uint32_t line = 1;
    uint32_t col  = 1;
    std::string_view filename;

    static SourceLocation unknown() { return {0, 0, "<unknown>"}; }
};

} // namespace kern
