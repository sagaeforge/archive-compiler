#pragma once

#include <rapidjson/document.h>
#include <unicode/unistr.h>

namespace nugdev::compiler::ast {

class ASTNodeDebugAspect {
  public:
    virtual rapidjson::Value create_debug_info() const = 0;
};

} // namespace nugdev::compiler::ast