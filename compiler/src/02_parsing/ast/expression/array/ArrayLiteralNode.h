#pragma once

#include "01_tokenize/Token.h"
#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::expression {

class ArrayLiteralNode : public Expression {
  public:
    ArrayLiteralNode(const tokenize::Token &token, const std::vector<std::shared_ptr<Expression>> &elements);

  public:
    virtual icu::UnicodeString to_str() const override;
    virtual json::JsonValue to_json(json::JsonAllocator &allocator) const override;
    virtual const tokenize::Token &get_token() const override;

  public:
    const std::vector<std::shared_ptr<Expression>> &get_elements() const;

  private:
    tokenize::Token m_token;
    std::vector<std::shared_ptr<Expression>> m_elements;
};

} // namespace nugdev::compiler::ast::expression
