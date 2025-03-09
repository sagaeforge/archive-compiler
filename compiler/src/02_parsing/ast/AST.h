#pragma once

#include "00_app/json/Json.hpp"
#include "00_app/lib/PointerHelper.hpp"
#include "01_tokenize/Token.h"

#include <memory>
#include <unicode/unistr.h>

namespace nugdev::compiler::ast {

class ASTNodeVisitor;

class ASTNode : public lib::PointerHelper<ASTNode> {
  public:
    virtual ~ASTNode() = default;

  public:
    void accept(const std::shared_ptr<ASTNodeVisitor> &visitor);

  public:
    virtual icu::UnicodeString to_str() const = 0;
    virtual json::JsonValue to_json(json::JsonAllocator &allocator) const = 0;
    virtual const tokenize::Token &get_token() const = 0;
};
using ASTNodePtr = std::shared_ptr<ASTNode>;

class Expression : public ASTNode {};
using ExpressionPtr = std::shared_ptr<Expression>;

class Statement : public ASTNode {};
using StatementPtr = std::shared_ptr<Statement>;

class Module : public ASTNode {};
using ModulePtr = std::shared_ptr<Module>;

} // namespace nugdev::compiler::ast
