#pragma once

#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::expression {

class IdentifierLiteralNode : public Expression {
  public:
    IdentifierLiteralNode(const tokenize::Token &token, icu::UnicodeString value);

  public:
    virtual icu::UnicodeString to_str() const override;
    virtual json::JsonValue to_json(json::JsonAllocator &allocator) const override;
    virtual const tokenize::Token &get_token() const override;

  private:
    tokenize::Token m_token;
    icu::UnicodeString m_value;
};

} // namespace nugdev::compiler::ast::expression