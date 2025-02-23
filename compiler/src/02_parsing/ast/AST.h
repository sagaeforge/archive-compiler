#pragma once

#include "00_app/json/Json.hpp"
#include "00_app/lib/PointerHelper.hpp"
#include "01_tokenize/Token.h"

#include <memory>

#include <unicode/unistr.h>

namespace nugdev::compiler {

namespace ast {

class ASTNodeVisitor;

class ASTNode : public lib::PointerHelper<ASTNode> {
  public:
    virtual ~ASTNode() = default;

  public:
    virtual std::shared_ptr<ASTNode> accept(std::shared_ptr<ASTNodeVisitor> &visitor);

  public:
    virtual icu::UnicodeString to_str() const = 0;
    virtual json::JsonValue to_json(json::JsonAllocator &allocator) const = 0;
    virtual const tokenize::Token &get_token() const = 0;
};

class Expression : public ASTNode {};
class Statement : public ASTNode {};
class Module : public ASTNode {};

} // namespace ast

} // namespace nugdev::compiler
