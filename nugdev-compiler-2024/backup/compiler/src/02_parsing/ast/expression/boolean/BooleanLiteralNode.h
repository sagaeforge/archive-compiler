#pragma once

#include "01_tokenize/Token.h"
#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::expression {

class BooleanLiteralNode : public Expression {
  public:
    BooleanLiteralNode(const tokenize::Token &token, bool value);

  public:
    virtual const tokenize::Token &get_token() const override;
    virtual TypeInfo get_type_info() const override;

  public:
    bool get_value() const;

  private:
    tokenize::Token m_token;
    bool m_value;
};
} // namespace nugdev::compiler::ast::expression