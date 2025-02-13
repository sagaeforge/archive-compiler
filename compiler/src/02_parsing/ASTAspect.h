#pragma once

#include <rapidjson/document.h>
#include <unicode/unistr.h>

namespace nugdev::compiler::parsing {

class ASTNodeDebugAspect {
  public:
    virtual bool create_metadata(const icu::UnicodeString &dir_path) const = 0;
    virtual rapidjson::Value create_debug_info() const = 0;
};

} // namespace nugdev::compiler::parsing