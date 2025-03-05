#pragma once

#include "01_tokenize/Token.h"
#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::expression {

class BooleanLiteralNode : public Expression {
  public:
    BooleanLiteralNode(const tokenize::Token &token, bool value);

  public:
    virtual icu::UnicodeString to_str() const override;
    virtual json::JsonValue to_json(json::JsonAllocator &allocator) const override;
    virtual const tokenize::Token &get_token() const override;

  public:
    bool get_value() const;

  private:
    tokenize::Token m_token;
    bool m_value;
};
} // namespace nugdev::compiler::ast::expression