#pragma once

#include "00_app/lib/PointerHelper.hpp"

#include <unicode/unistr.h>

namespace nugdev::compiler::parsing {

class ASTNode : public lib::PointerHelper<ASTNode> {
  public:
    virtual ~ASTNode() = default;

  public:
    virtual icu::UnicodeString to_str() const = 0;
};

class Expression : public ASTNode {};
class Statement : public ASTNode {};
class Module : public ASTNode {};

} // namespace nugdev::compiler::parsing
