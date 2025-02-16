#pragma once

#include "00_app/lib/PointerHelper.hpp"

#include <memory>
#include <vector>

#include <unicode/unistr.h>

namespace nugdev::compiler::ast {

class ASTNodeVisitor;

class ASTNode : public lib::PointerHelper<ASTNode> {
  public:
    virtual ~ASTNode() = default;

  public:
    virtual std::shared_ptr<ASTNode> accept(std::shared_ptr<ASTNodeVisitor> &visitor) = 0;

  public:
    virtual icu::UnicodeString to_str() const = 0;
};

class Expression : public ASTNode {};
class Statement : public ASTNode {};
class Module : public ASTNode {};

} // namespace nugdev::compiler::ast
