#pragma once

#include <unicode/unistr.h>

#include "00_app/json/Json.hpp"

namespace nugdev::compiler::ast {

class ASTNodeDebugAspect {
  public:
    virtual json::JsonValue create_debug_info(json::JsonAllocator &allocator) const = 0;
};

} // namespace nugdev::compiler::ast